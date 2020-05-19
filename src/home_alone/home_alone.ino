/*
  Home Alone Application

  by Peter B, from Switzerland
  Project website http://projects.descan.com/projekt7.html


  After hearing an number of stories about old people dying in their appartements I was looking for a simple solution, then, after a while
  I came across a project presented by Ralph Bacon, check it out here:
  https://youtu.be/5IxOUxZFfnk

  I downloaded his code from github but found it somewhat hard to read - being just one long piece of code. Well documented but still
  hard to read.
  Since I wanted to use an ESP32 I started from scratch.

  My Project will run on an ESP32 so therefore use is made of multitasking.

  The Application is modeled with a State Machine - this is the only clean solution that will prevent lot's of switches which are in fact
  implementing a state machine the ugly way.

  I also use Adreas Spiess Debug Prepocessor directives (to keep production code small).
  Check it out here : .https://youtu.be/1eL8dXL3Wow

  I started out with having a state NIGHT with no reporting (same as Ralph Bacon) but later decided to remove this state
  and have reporting for the whole 24 hours. Thingspeak is used to report the number of movements to

  Instead of email I opted to use pushover messages for a daily reporting to a smartphone and iPad.

  The whole thing is built with Adafruit Feather components. I encountered problems with the OLED display.
  I tried several versions of oled libs, including the one from Adafruit. There seem to be timing problems with the ESP32. See the web - they all
  work well with ESP8266 but not with ESP32.
  Finally I found this lib to work ok, I followed the following description:
  https://techtutorialsx.com/2017/12/02/esp32-arduino-interacting-with-a-ssd1306-oled-display/

  This LIb is used for the OLDE display:
  ESP8266_and_ESP32_OLED_driver_for_SSD1306_displays
  The OLED display is on for 50 sec. It can be switched on for 50 sec. by the press of a button.
  Another button is used to force the sending of a test pushover message.

  Two led are used: green led simply blinking when the system is running, red led used to show detection of movement.

  4 Task are used, the task running the setup() is terminated after setup is complete.

  The config file is read from an micro sd card.
  The OLED display is uncluttered, showing simply the state of the last connectin to Thingsspeak and Pushover respectively.
  Pushover can be found here:
  myPushover Lib on Github
  https://github.com/dakota127/myPushover

*/
//----------------------------------------------------------------

// degugging stuff ------------------------------
#define DEBUGLEVEL 0        // für Debug Output, for production set this to DEBUGLEVEL 0  <---------------------------
#include <DebugUtils.h>     // Library von Adreas Spiess

#define DEBUGPUSH true      // für Debug push messages, set to 1 for messages of important events


#include <time.h>
#include <WiFi.h>                 // used for thingspeak
#include <WiFiClientSecure.h>     // used for pushover
#include "ThingSpeak.h"
#include <ArduinoJson.h>
#include <SD.h>
//  siehe beispiele https://github.com/espressif/arduino-esp32/issues/449
#include <rom/rtc.h>
#include <myPushover.h>         // my Pushover Library

// Libraries for Logging on SD card
#include "FS.h"
#include "SD.h"

//--- helper macros ---------- number of items in an array
#define NUMITEMS(arg) ((unsigned int) (sizeof (arg) / sizeof (arg [0])))

//---- Input-Output (gpio) related definitions and variables ----
#define redledPin 27        // choose the pin for the LED
#define greenledPin 12      // choose the pin for the LED
#define inputPin 21         // choose the input pin (for PIR sensor)
#define button_awayPin 14   // choose the input pin  (A)
#define button_oledPin 15   // choose the input pin  (C)
#define button_test 32      // choose the input pin  (C)


//-----  Task related definitions and variables ----
TaskHandle_t Task1;
TaskHandle_t Task2;
TaskHandle_t Task3;
TaskHandle_t Task4;

SemaphoreHandle_t SemaButton;
SemaphoreHandle_t SemaMovement;
SemaphoreHandle_t SemaOledSignal;
SemaphoreHandle_t wifi_semaphore = NULL;

SemaphoreHandle_t clock_1Semaphore;
SemaphoreHandle_t clock_2Semaphore;

static volatile bool clock_tick_1 = false;
static volatile bool clock_tick_2 = false;

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
  int priority;
  String pushtext;
} wifi_order_struct;

// static volatile wifi_struct  wifi_order_struct;

bool wifiStatus;

// WiFiClientSecure  client_push;  nicht verwendet
WiFiClient        client_thing;

//---- State Machine related definitions and variables ----
enum {
  // finite state machins states
  ATHOME,
  LEAVING,
  AWAY
};

uint8_t state = ATHOME;              // state variable is global

//---- communication between tasks ----------------
static volatile unsigned long movCount_reportingPeriod_cloud = 0;       //  count per reporting period
static volatile unsigned long movCount_reportingPeriod_push = 0;        //  count per reporting period


static volatile unsigned long  button_awaypressed = 0;
static volatile unsigned long button_oledpressed = 0;
static volatile time_t timelastMovement ;

//---- display related definitions and variables ----
// 3 variable for Oled display
static volatile int value1_oled = 9 ;             // 1 if at least one wifi connection was successful
static volatile int value2_oled = 9 ;             // last wifi connection: 1 was ok, 7 was not successful
static volatile int value3_oled = 9 ;             // last report to cloud: 1 was ok, 7 was not successful
static volatile int value4_oled = 5 ;             // 1 = at home, 2= leaving, 3: away , 4 : night, 5: init
static volatile int value5_oled = 0 ;             // movement count
int oledsignal = 1;

static volatile int credential_to_use = 0;        // 0: try all credentials
#define WIFI_DETAILS 0

//---- Config related definitions and variables ----
struct Config {               /// config struct
  int ThingSpeakChannelNo;
  int ThingSpeakFieldNo;
  char ThingSpeakWriteAPIKey[20];
  char Title [20];
  char PersonName [20];
  int MinutesBetweenUploads;
  char wlanssid_1[20];
  char wlanpw_1[15];
  char wlanssid_2[20];
  char wlanpw_2[15];
  char NTPPool[20];
  char Timezone_Info[60];     // enter your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)
  char Email_1[20];
  char Email_2[20];
  int TimeOutLeavingSec;
  int MaxActivityCount;
  int ScreenTimeOutSeconds;
  char PushoverUserkey[32];
  char PushoverToken[32];
  char PushoverDevices[30];
  int HoursbetweenNoMovementRep;
  int EveningReportingHour;
  int MorningReportingHour;

};
char *credentials[] = { "", "", "", "" };
const char *filename = "/config.json";      // <- SD library uses 8.3 filenames
Config config;                              // <- global configuration object

const char* path = "/log.txt";              // logfile on sd card
String logMessage;

#define STATE_LEAVE_TEST      120           // in seconds for test debug
#define UPLOAD_INTERVALL_TEST  300          // in seconds for test debug
#define PUSHOVER_INTERVALL_TEST  900        // in seconds for test debug
#define HOURSBETWEENNOMOV   1               // 1 hour

//---- Time related definitions and variables ----
tm timeinfo;
time_t now;
long unsigned lastNTPtime;
unsigned long lastEntryTime;
char currTime[80];
String timeStamp;

//boot info
char time_lastreset[30];
int   lastResetreason;
char  reset_reason[30];

// time-date stuff
static volatile int curr_hour;
static volatile int curr_dayofyear;
static volatile int curr_year;                  // function returns year since
static volatile int old_hour;
static volatile int old_dayofyear;
static volatile int old_year;                   // function returns year since
static bool done_morningreporting = false;      // have we already done it at the specified hour ?
static bool done_eveningreporting = false;
static volatile bool is_newday = false;




//---- Function Prototypes -----------------------
void printLocalTime();
int wifi_connect (char *, char * , int);
void wifi_disconnect ();
void task_detect( void *);
void state_machine( void *);
void task_wifi( void *);
void task_display (void *);
void task_clock (void *);
void do_athome();
void do_away();
void do_leaving();
bool report_toCloud();
int loadConfig ();
int wifi_func();

//---- This and that -------------------------------

int debug_flag = false;                 // for output to serial console, is set if DEBUGLEVEL is > 0
int debug_flag_push = DEBUGPUSH;        // for test push messages  set manually here !!

int pirState = LOW; // we start, assuming no motion detected
int val = 0; // variable for reading the pin status
int ret = 0;


//------------------------------------------------
// displays at startup the Sketch running in the Arduino
void display_Running_Sketch (void) {
  String the_path = __FILE__;
  int slash_loc = the_path.lastIndexOf('/');
  String the_cpp_name = the_path.substring(slash_loc + 1);
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

  clock_1Semaphore = xSemaphoreCreateMutex();
  clock_2Semaphore = xSemaphoreCreateMutex();

  Serial.begin(115200);
  vTaskDelay(3000 / portTICK_PERIOD_MS);     // just wait
  display_Running_Sketch();

  Serial.print ("CPU0 reset reason: ");
  store_reset_reason(rtc_get_reset_reason(0));
  Serial.println (reset_reason);
  lastResetreason = rtc_get_reset_reason(0);

  Serial.print ("CPU1 reset reason: ");
  store_reset_reason(rtc_get_reset_reason(1));
  Serial.println (reset_reason);

  WiFi.mode(WIFI_STA);
  if (DEBUGLEVEL > 0) {
    debug_flag = true;                          // some functions need this
    Serial.println("----- debug on ---------");
  }

  DEBUGPRINTLN1 ("start setup");

  //  digitalWrite(redledPin, LOW);             // turn LED OFF



  movCount_reportingPeriod_push = 0;          // no semaphore needed, no other task running


  DEBUGPRINTLN2 ("about to start task1");
  vTaskDelay(100 / portTICK_PERIOD_MS);     // start oled task first on core 1

  // --------- start function do_oled in a separate task --------------------------------------
  // parameters to start a task:
  //  (name of task , Stack size of task , parameter of the task , priority of the task , Task handle to keep track of created task , core )
  //
  xTaskCreatePinnedToCore (task_display, "oledtask", 3000, NULL, TASK_PRIORITY, &Task1, CORE_1);

  vTaskDelay(200 / portTICK_PERIOD_MS);

  int ret = logInit(path);

  // load configdata into struct --------------------------
  ret = loadConfig(filename, config);       // load config from json file

  WiFi.mode(WIFI_STA);                      // Wifi mode is station
  ThingSpeak.begin(client_thing);                 // Initialize ThingSpeak

  vTaskDelay(200 / portTICK_PERIOD_MS);

  Serial.println ("calling initial test wifi");
  // set up parameter for this job       Text Wifi
  wifi_todo = TEST_WIFI;
  wifi_order_struct.order = wifi_todo;
  ret = wifi_func();

  // init time related stuff
  curr_hour = timeinfo.tm_hour;
  curr_dayofyear = timeinfo.tm_yday;
  curr_year = timeinfo.tm_year - 100;       // function returns year since 1900
  old_dayofyear = curr_dayofyear;
  old_year = curr_year  ;                   // function returns year since 1900


  strftime(time_lastreset, 30, "%a %d.%m.%y %T", localtime(&now));
  if (debug_flag) {
    Serial.println("");
    Serial.print ("Boot Time: ");
    Serial.println(time_lastreset);
  }
  // create other tasks ------------

  DEBUGPRINTLN2 ("about to start task2");      // clock
  //  task can only be started after time is available...
  vTaskDelay(100 / portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore ( task_clock, "clock", 5000, NULL, TASK_PRIORITY, &Task2, CORE_1);

  vTaskDelay(200 / portTICK_PERIOD_MS);

  DEBUGPRINTLN2 ("about to start task3");
  vTaskDelay(200 / portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore (task_detect, "Movement", 2000, NULL, TASK_PRIORITY, &Task3, CORE_1);

  vTaskDelay(200 / portTICK_PERIOD_MS);

  DEBUGPRINTLN2 ("about to start task4");
  vTaskDelay(100 / portTICK_PERIOD_MS);

  // Tast state machine  - the main loop
  xTaskCreatePinnedToCore ( state_machine, "STM", 10000, NULL, TASK_PRIORITY, &Task4, CORE_1);

  vTaskDelay(200 / portTICK_PERIOD_MS);

  // end creating tasks ------------------


  DEBUGPRINTLN1 ("Setup done...");


  getTimeStamp();
  getCurrTime(false);
  logMessage = String(currTime) + "," +  "Setup ended" + "\r\n";
  log_SDCard(logMessage, path);
  vTaskDelay(700 / portTICK_PERIOD_MS);
  vTaskDelete(NULL);                    // delete this initial setup task
}


//----------------------------------------------------
//  Loop, runs on core 1 by default -------------------
void loop() {

  delay(1000);
}
//--------------------------------------------
// end of code
//--------------------------------------------
//
