/*
* Home Alone Application
* This project is based on a project presentd by Ralph Bacon, check it out here:
* 
* https://youtu.be/5IxOUxZFfnk
* https://www.youtube.com/redirect?redir_token=8C-a1g2b7KcNYW5kGnk-NXcvkRV8MTU3OTExMDA4NEAxNTc5MDIzNjg0&event=video_description&v=5IxOUxZFfnk&q=https%3A%2F%2Fgithub.com%2FRalphBacon%2FESP8266-Home-Alone-Vulnerable-Person-Monitor
* 
* I tried to understand and unclutter his code but that proved to be futile. So I started from scratch.
* 
* My Project will run on an ESP32 so therefore use is made of multitasking.
* 
* The Application is modeled with a State Machine - this is the only clean solution that will prevent lot's of switches which are in fact
* implementing a state machine the ugly way. 
* 
* I also use Adreas Spiess Debug Prepocessor directives.
* Check it out here : .https://youtu.be/1eL8dXL3Wow
* 
* Pushover Lib on Github
* https://github.com/ArduinoHannover/Pushover
* https://alexbloggt.com/esp8266-pushover/
* 
*/
//----------------------------------------------------------------
#define ESP32

#include <time.h>
#include <WiFi.h>
#include "ThingSpeak.h"
#include <ArduinoJson.h>
#include <SD.h>
#define DEBUGLEVEL 0        // für Debug Output, for production set this to DEBUGLEVEL 0  <---------------------------
#include <DebugUtils.h>     // Library von Adreas Spiess

#define  REPORTING_TRIGGERLEVEL  40              // to be added to config File later <----- 

//--- helper macros ---------- number of items in an array
#define NUMITEMS(arg) ((unsigned int) (sizeof (arg) / sizeof (arg [0])))

//---- Input-Output (gpio) related definitions and variables ----
#define redledPin 27        // choose the pin for the LED
#define greenledPin 12     // choose the pin for the LED
#define inputPin 33        // choose the input pin (for PIR sensor)
#define button_awayPin 14        // choose the input pin  (A)
#define button_oledPin 15        // choose the input pin  (C)
#define button_test 32        // choose the input pin  (C)



//-----  Task related definitions and variables ----
TaskHandle_t Task1;
TaskHandle_t Task2;
TaskHandle_t Task3;
TaskHandle_t Task4;
TaskHandle_t Task5;

SemaphoreHandle_t SemaButton;
SemaphoreHandle_t SemaMovement;
SemaphoreHandle_t SemaOledSignal;
SemaphoreHandle_t wifi_semaphore = NULL;

SemaphoreHandle_t clock_1Semaphore;
SemaphoreHandle_t clock_2Semaphore;
SemaphoreHandle_t clock_3Semaphore;
static volatile bool clock_tick_1 = false;
static volatile bool clock_tick_2 = false;
static volatile bool clock_tick_3 = false;


#define CORE_0  0                   // core to run on
#define CORE_1  1
#define TASK_PRIORITY 1

enum {
    TEST_WIFI,                                      // finite state machins states
    REPORT_CLOUD,
    PUSH_MESG
    };
    
int wifi_todo;

struct wifi_struct {               /// wifi struct
  int order;
  int mvcount;
  String pushtext;
} wifi_order_struct;

// static volatile wifi_struct  wifi_order_struct;


//---- communication between tasks ----------------
static volatile unsigned long movement_count = 0;
static volatile unsigned long movement_count_perday = 0;
static volatile unsigned long button_awaypressed = 0;
static volatile unsigned long button_oledpressed = 0;

//---- display related definitions and variables ----
// 3 variable for Oled display
static volatile int value1_oled = 9 ;             // 1 if at least one wifi connection was successful
static volatile int value2_oled = 9 ;             // last wifi connection: 1 was ok, 7 was not successful
static volatile int value3_oled = 9 ;             // last report to cloud: 1 was ok, 7 was not successful
static volatile int value4_oled = 5 ;             // 1 = at home, 2= leaving, 3: away , 4 : night, 5: init
static volatile int value5_oled = 0 ;             // movement count
int oledsignal = 1;


//---- State Machine related definitions and variables ----
enum {
    NIGHT,                                      // finite state machins states
    ATHOME,
    LEAVING,
    AWAY
    };
uint8_t state = ATHOME;              // state variable

//---- Network related definitions and variables ----
WiFiClient  client;


static volatile int credential_to_use = 0;        // 0: try all credentials
#define WIFI_DETAILS 0

//---- Config related definitions and variables ----
struct Config {               /// config struct
  int ThingSpeakChannelNo;
  int ThingSpeakFieldNo;
  char ThingSpeakWriteAPIKey[20];
  char PersonName [20];
  int MinutesBetweenUploads;
  char wlanssid_1[12];
  char wlanpw_1[15];
  char wlanssid_2[12];
  char wlanpw_2[15];  
  char Timezone_Info[60];     // enter your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)
  char NTPPool[20];
  char Email_1[20];
  char Email_2[20];
  int emailTriggerLevel;
  int MinutesBetweenEmails;
  int TimeOutPeriodSec;
  int MaxActivityCount;
  int BeepOnMovement;
  int QuietHoursStart;
  int QuietHoursEnd;
  int ScreenTimeOutSeconds;
  char PushoverUserkey[30];
  char PushoverToken[30];
  int MinutesBetweenPushover;
  int EveningReportingHour;
  int MorningReportingHour;
  int ReportingThreshold;
 
};
char *credentials[] = { "", "", "", "" };
const char *filename = "/config.json";  // <- SD library uses 8.3 filenames
Config config;                         // <- global configuration object

#define MAIL_INTERVALL_TEST  900       // in seconds for test debug
#define STATE_LEAVE_TEST      120       // in seconds for test debug
#define UPLOAD_INTERVALL_TEST  300     // in seconds for test debug
#define PUSHOVER_INTERVALL_TEST  900     // in seconds for test debug

//---- Time related definitions and variables ----
tm timeinfo;
time_t now;
long unsigned lastNTPtime;
unsigned long lastEntryTime;

static volatile int day_night_old_dayofyear;
static volatile int day_night_old_year;
static volatile int old_dayofyear;
static volatile int old_year;
static volatile int old_hour;

static volatile int curr_hour;
static volatile int curr_dayofyear;
static volatile int curr_year;   // function returns year since 


//keep this ....
//#define ALARM_DURATION_1  340000  // (12'000 sind 2 Min) 34'000 sind ca. 6 Min)
//#define ALARM_DURATION_2  120000  // (12'000 sind 2 Min) 34'000 sind ca. 6 Min)


//---- Function Prototypes -----------------------
void printLocalTime();
int wifi_connect (char *, char * , int);
void wifi_disconnect ();
void detectMovement( void *);
void do_athome();
void do_away();
void do_leaving();
int report_toCloud();


//---- This and that -------------------------------
int debug_flag = false;
int pirState = LOW; // we start, assuming no motion detected
int val = 0; // variable for reading the pin status
int ret = 0;


//------------------------------------------------
// displays at startup the Sketch running in the Arduino
void display_Running_Sketch (void){
  String the_path = __FILE__;
  int slash_loc = the_path.lastIndexOf('/');
  String the_cpp_name = the_path.substring(slash_loc+1);
  int dot_loc = the_cpp_name.lastIndexOf('.');
  String the_sketchname = the_cpp_name.substring(0, dot_loc);

  Serial.print("\n************************************");
  Serial.print("\nRunning Sketch: ");
  Serial.println(the_sketchname);
  Serial.print("Compiled on: ");
  Serial.print(__DATE__);
  Serial.print(" at ");
  Serial.print(__TIME__);
  Serial.print("\n");
  Serial.print("************************************\n");
}



//----------------------------------------------------------
// SETUP ---------------------------------------------------
//----------------------------------------------------------
void setup() {
  pinMode(redledPin, OUTPUT);             // declare LED as output
  pinMode(greenledPin, OUTPUT);           // declare LED as output
  pinMode(inputPin, INPUT);               // declare as input
  pinMode(button_awayPin, INPUT_PULLUP);  // declare button as input
  pinMode(button_oledPin, INPUT_PULLUP);  // declare button as input
  pinMode(button_test, INPUT_PULLUP);  // declare button as input
  
  // Semaphores init
  SemaMovement  = xSemaphoreCreateMutex();
  SemaButton    = xSemaphoreCreateMutex();
  SemaOledSignal = xSemaphoreCreateMutex();
  wifi_semaphore = xSemaphoreCreateMutex();
  
  Serial.begin(115200);
  delay(2000);   //wait so we see everything
  display_Running_Sketch();

  if (DEBUGLEVEL > 0) {
    debug_flag = true;                          // some functions need this
    Serial.println("----- debug on ---------");
   }

  DEBUGPRINTLN1 ("start setup");

  digitalWrite(redledPin, LOW); // turn LED OFF

  old_dayofyear  = 0;
  old_year = 0;
  day_night_old_dayofyear = 0;
  day_night_old_year = 0;
 
           
  DEBUGPRINTLN1 ("about to start task1");
  vTaskDelay(100 / portTICK_PERIOD_MS);    // start oled task first on core 1

// --------- start function do_oled in a separate task -------------------------------------- 
// parameters to start a task:
//  (name of task , Stack size of task , parameter of the task , priority of the task , Task handle to keep track of created task , core )
//
  xTaskCreatePinnedToCore (do_oled, "oledtask", 2000, NULL, TASK_PRIORITY, &Task1, CORE_1);

  vTaskDelay(200 / portTICK_PERIOD_MS);


// load configdata into struct --------------------------
  ret = loadConfig(filename, config);     // load config from json file

  WiFi.mode(WIFI_STA);          // Wifi mode is station          
  ThingSpeak.begin(client);     // Initialize ThingSpeak
  
 // --------- start function wifi in a separate task -------------------------------------- 
// parameters to start a task:
//  (name of task , Stack size of task , parameter of the task , priority of the task , Task handle to keep track of created task , core )
//
  
  xTaskCreatePinnedToCore (wifi_task, "wifitask", 6000, NULL, TASK_PRIORITY, &Task1, CORE_1);

  vTaskDelay(200 / portTICK_PERIOD_MS);


  state = ATHOME;                   // initial state of state machine

 // we need to test wifi - this is done in the wifi task
 // first we need to se if task is free or busy - we check the tasks semaphore

 
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait 10 ticks to see if it becomes free. */
        if( xSemaphoreTake( wifi_semaphore, ( TickType_t ) 10 ) == pdTRUE )
        {
            /* We were able to obtain the semaphore and can now access the
            shared resource. */
         // set up parameter for this job
             wifi_todo = TEST_WIFI;
             wifi_order_struct.order = wifi_todo;
            vTaskResume( Task1 );
            /* We have finished accessing the shared resource.  Release the
            semaphore. 
            No: the wifi task is freeing it */
        }
        else
        {
            /* We could not obtain the semaphore and can therefore not access
            the shared resource safely. */

            DEBUGPRINTLN2 ("wifi semaphore busy");
            vTaskDelay(200 / portTICK_PERIOD_MS);

        }
  
  // we wait for the initial wifi connection to finish, after that we continue with the setup

  bool wifi_not_done = true;

  while (wifi_not_done){
         /* See if we can obtain the semaphore.  If the semaphore is not
        available wait 10 ticks to see if it becomes free. */
        if( xSemaphoreTake( wifi_semaphore, ( TickType_t ) 100 ) == pdTRUE )
        {
            /* We were able to obtain the semaphore and can now access the */
     
           xSemaphoreGive( wifi_semaphore );

           wifi_not_done = false;
        }
        else
        {
            /* We could not obtain the semaphore and can therefore not access
            the shared resource safely. */

            DEBUGPRINTLN2  ("wifi semaphore busy (setup)");
            vTaskDelay(400 / portTICK_PERIOD_MS);

        }

  }
   
// create other tasks ------------

  DEBUGPRINTLN1 ("about to start task2");             // metronome 
//  task can only be started after time is available...
  vTaskDelay(100 / portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore ( metronome,"metronome", 5000, NULL, TASK_PRIORITY, &Task2, CORE_1);

  vTaskDelay(200 / portTICK_PERIOD_MS);

   DEBUGPRINTLN1 ("about to start task3");
   vTaskDelay(100 / portTICK_PERIOD_MS);
   xTaskCreatePinnedToCore (detectMovement, "Movement", 2000, NULL, TASK_PRIORITY, &Task3, CORE_0);                 

   vTaskDelay(200 / portTICK_PERIOD_MS);

   DEBUGPRINTLN1 ("about to start task4");
   vTaskDelay(100 / portTICK_PERIOD_MS);

// Tast state machine  - the main loop
   xTaskCreatePinnedToCore ( state_machine, "STM", 6000, NULL, TASK_PRIORITY, &Task4, CORE_1);

   vTaskDelay(200 / portTICK_PERIOD_MS);

// end creating tasks ------------------
  

  DEBUGPRINTLN1 ("Setup done...");
}

//-----------------------------------------------------
// implementation of the state machine (Runs in separate task !)
//-----------------------------------------------------
void state_machine( void * parameter )
{
 static bool main_firsttime = true ;
  for (;;) {

    if (main_firsttime) {
      DEBUGPRINT1 ("TASK state_machine - Running on core:");
      DEBUGPRINTLN1 (xPortGetCoreID());
      main_firsttime = false;
    }
 
 // switch according to state variable
  switch (state) {

    case ATHOME:
      do_athome();
      break;

    case LEAVING:
      do_leaving();
      break;

    case AWAY:
      do_away();
      break;
  
    case NIGHT:
      do_night();
      break;

    default:
      DEBUGPRINTLN0 ("case main zu default, error!");
      vTaskDelay(200 / portTICK_PERIOD_MS);
      break;
   }              // end case stmt

    vTaskDelay(200 / portTICK_PERIOD_MS);

  }
}




//----------------------------------------------------
//  Loop, runs on core 1 by default -------------------
void loop(){
  
  delay(1000);
}
//--------------------------------------------
