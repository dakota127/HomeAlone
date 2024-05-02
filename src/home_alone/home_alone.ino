/*
  Home Alone Application

  by Peter B, from Switzerland
  Project website http://projects.descan.com/projekt7.html

  First Version 2020
  Current Version 2024

  After hearing an number of stories about old people dying in their appartements I was looking for a simple solution, then, after a while
  I came across a project presented by Ralph Bacon, check it out here:
  https://youtu.be/5IxOUxZFfnk

  I downloaded his code from github but found it somewhat hard to read - being just one long piece of code. Well documented but still
  hard to read.
  Since I wanted to use an ESP32 I started from scratch.

  My Project will run on an ESP32 so therefore use is made of multitasking.

  The Application is modeled with a State Machine - this is the only clean solution that will prevent lot's of switches which are in fact
  implementing a state machine the ugly way.

  I also use Andreas Spiess Debug Prepocessor directives (to keep production code small).
  Check it out here : .https://youtu.be/1eL8dXL3Wow

  I started out with having a state NIGHT with no reporting (same as Ralph Bacon) but later decided to remove this state
  and have reporting for the whole 24 hours. Thingspeak is used to report the number of movements to

  Instead of email I opted to use pushover messages for a daily reporting to a smartphone and iPad.

  The whole thing is built with Adafruit Feather components. 

  The OLED display is on for 50 sec. It can be switched on for 50 sec. by the press of a button.
  Another button is used to force the sending of a test pushover message.

  Two led are used: green led simply blinking when the system is running, red led used to show detection of movement.

  4 Task are used, the task running the setup() is terminated after setup is complete.

  The config file is read from an micro sd card.
  The OLED display is uncluttered, showing simply the state of the last connectin to Thingsspeak and Pushover respectively.

*/
//----------------------------------------------------------------

// degugging stuff ------------------------------
// set this to 1, 2 or 3 for increasing debug output
// set this to 0 for production
#define DEBUGLEVEL 0  // controls Debug Output  <---------------------------
// ----------------------------------------------------------
#include <time.h>
#include <WiFi.h>              // used for thingspeak
#include <WiFiClientSecure.h>  // used for pushover
#include "ThingSpeak.h"
#include <ArduinoJson.h>
#include "home_alone.h"

// Libraries for Logging on SD card
#include <FS.h>
#include <SD.h>

//---- Input-Output (gpio) related definitions and variables ----
#define redledPin 27       // choose the pin for the LED
#define greenledPin 12     // choose the pin for the LED
#define sensorPin 37       // choose the input pin (for PIR sensor) war 21
#define button_awayPin 14  // choose the input pin  (A)
#define button_oledPin 15  // choose the input pin  (C)
#define button_test 32     // choose the input pin  (C)

//-----  Task related definitions and variables ----
TaskHandle_t Task1;
TaskHandle_t Task2;
TaskHandle_t Task3;
TaskHandle_t Task4;

// Semaphores used
SemaphoreHandle_t SemaButton;
SemaphoreHandle_t SemaMovement;
SemaphoreHandle_t SemaOledSignal;
SemaphoreHandle_t wifi_semaphore = NULL;
SemaphoreHandle_t clock_1Semaphore;
SemaphoreHandle_t clock_2Semaphore;

static volatile bool clock_tick_1 = false;
static volatile bool clock_tick_2 = false;

#define CORE_0 0  // core to run on
#define CORE_1 1
#define TASK_PRIORITY 1

enum wificmd {
  DO_WIFI,  // finite state machins states
  THINGSPEAK,
  PUSHOVER
};
enum wificmd wifi_todo;

struct wifi_struct {  /// wifi struct to communicate with wifi function
  int order;
  int mvcount;
  int priority;
  String pushtext;
} wifi_order_struct;


bool wifiStatus;

WiFiClient client_thing;

//---- State Machine related definitions and variables ----
enum machine_states {
  // finite state machins states
  ATHOME,
  LEAVING,
  AWAY
};
enum machine_states state;  // state variable is global

//---- Movement Counters global ----------------
static volatile unsigned long movCount_counter_1 = 0;  //  count per reporting period
static volatile unsigned long movCount_counter_2 = 0;   //  count per daily reporting
static volatile unsigned long alert_reporting_counter = 1;   //  number of alert messages

static volatile unsigned long button_awaypressed = 0;
static volatile unsigned long button_oledpressed = 0;
static volatile time_t timelastMovement;

//---- display related definitions and variables ----
// 3 variable for Oled display
static volatile int value1_oled = 9;  // 1 if at least one wifi connection was successful
static volatile int value2_oled = 9;  // last wifi connection: 1 was ok, 7 was not successful
static volatile int value3_oled = 9;  // last report to cloud: 1 was ok, 7 was not successful
static volatile int value4_oled = 5;  // 1 = at home, 2= leaving, 3: away , 4 : night, 5: init
static volatile int value5_oled = 0;  // movement count

enum display_show oledsignal;  // used to signal to the display task

unsigned long previousMillis = 0;  // will store last time LED was updated

//---- Config related definitions and variables ----
struct Config_struct {             /// config struct
  int ThingSpeakChannelNo;         // Channel number ThingSpeak
  int ThingSpeakFieldNo;           // Field number ThingSpeak
  char ThingSpeakWriteAPIKey[20];  // API Key ThingSpeak
  char Title[20];                  // Title für ThingSpeak Anzeige
  char PersonName[20];             // Name der überwachten Person
  int MinutesBetweenUploads;       // Minutes between reporting to ThingSpeak
  char wlanssid_1[20];             // SSID Wlan
  char wlanpw_1[25];               // PW Wlan
  char NTPPool[20];                // Pool Infp NTP Server
  char Timezone_Info[60];          // enter your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)
  char Email_1[20];                // Email Entwickler
  char Email_2[20];                // currently not used
  int TimeOutLeavingSec;           // Timeout after Press Leaving button
  int MaxActivityCount;            // Max number reported Movements
  int ScreenTimeOutSeconds;        // How many Seconds display is on
  char PushoverUserkey[32];        // User Key Pushover
  char PushoverToken[32];          // User Token Pushover
  char PushoverDevices[30];        // Devices Pushover
  int HoursbetweenNoMovementRep;   // How many hours between no movements
  int EveningReportingHour;        // Time for eveneing reporting
  int EssentialDebug;
};

Config_struct config;  // <- global configuration object of type config


char *credentials[] = { "", "", "", "" };
const char *filename = "/config.json";  // <- SD library uses 8.3 filenames

const char *path = "/log.txt";  // logfile on sd card
String logMessage;
char buffer1[80];
static bool display_control = false;


//---- Time related definitions and variables ----
tm timeinfo;
time_t aktuelle_epochzeit;
unsigned long lastEntryTime;
char currTime_string[80];
String timeStamp;

struct tm timestrukt;
//boot info
char time_lastreset[30];

bool essential_debug = false;  // value read from config file


// time-date stuff
static volatile int curr_hour;
static bool done_eveningreporting = false;


//---- Function Prototypes -----------------------
void printLocalTime();
//int wifi_connect(char *, char *, int);
//void wifi_disconnect();
void task_detect(void *);
void state_machine(void *);
void task_wifi(void *);
void task_display(void *);
void task_clock(void *);
void do_athome();
void do_away();
void do_leaving();
bool report_toCloud();
int loadConfig();
int wifi_func();

//---- This and that -------------------------------

int debug_flag = false;           // for output to serial console, is set if DEBUGLEVEL is > 1
int pirState = LOW;  // we start, assuming no motion detected
int val = 0;         // variable for reading the pin status
int ret = 0;


//------------------------------------------------
// displays at startup the Sketch running in the Arduino
void display_Running_Sketch(void) {
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

  Serial.begin(115200);
  vTaskDelay(3000 / portTICK_PERIOD_MS);  // just wait
  display_Running_Sketch();

  pinMode(redledPin, OUTPUT);             // declare LED as output
  pinMode(greenledPin, OUTPUT);           // declare LED as output
  pinMode(sensorPin, INPUT);              // declare as input
  pinMode(button_awayPin, INPUT_PULLUP);  // declare button as input
  pinMode(button_oledPin, INPUT_PULLUP);  // declare button as input
  pinMode(button_test, INPUT_PULLUP);     // declare button as input

  // Semaphores init
  SemaMovement = xSemaphoreCreateMutex();
  SemaButton = xSemaphoreCreateMutex();
  SemaOledSignal = xSemaphoreCreateMutex();
  wifi_semaphore = xSemaphoreCreateMutex();
  clock_1Semaphore = xSemaphoreCreateMutex();
  clock_2Semaphore = xSemaphoreCreateMutex();

  state = ATHOME;
 previousMillis = millis();  // note millis, might be used later...
  delay(1000);  //  do not rush things
  if (DEBUGLEVEL > 1) {
    debug_flag = true;  // some functions need this
    Serial.println("----- debug on ---------");
  }

  digitalWrite(redledPin, LOW);  // turn LED OFF

  movCount_counter_2 = 0;  // no semaphore needed, no other task running

  if (essential_debug) Serial.println("about to start task1 (display)");
  vTaskDelay(100 / portTICK_PERIOD_MS);  // start oled task first on core 1

  // --------- start function do_oled in a separate task --------------------------------------
  // this the first task to be started since we do need the oled display
  //  all other task are started once WiFi is ok and Local Time is ok.
  //
  //  parameters to start a task:
  //  (name of task , Stack size of task , parameter of the task , priority of the task , Task handle to keep track of created task , core )
  //
  oledsignal = START;  // oled task should display the text START
  xTaskCreatePinnedToCore(task_display, "oledtask", 4000, NULL, TASK_PRIORITY, &Task1, CORE_1);

  vTaskDelay(200 / portTICK_PERIOD_MS);

  //int ret = logInit(path);

  // load configdata into struct --------------------------
  ret = loadConfig(filename, config);  // load config from json file

  // add errrohandling here.....


  if (config.EssentialDebug == 1) essential_debug = true;


  ThingSpeak.begin(client_thing);  // Initialize ThingSpeak

  vTaskDelay(200 / portTICK_PERIOD_MS);

  xSemaphoreTake(SemaOledSignal, portMAX_DELAY);  // signal oled task to switch display on
  oledsignal = WIFI;
  xSemaphoreGive(SemaOledSignal);

  if (essential_debug) Serial.println("calling initial wifi connection");
  // set up parameter for this job       Text Wifi
  wifi_todo = DO_WIFI;
  wifi_order_struct.order = wifi_todo;
  ret = wifi_func();

  if (ret > 0) {
    // no initial wifi, we can not proceed
    Serial.println("No inital WiFi connection, restart ESp32");
    logMessage = String("NO WiFI, Restart ESP32\r\n");
    log_SDCard(logMessage, path);

    // if another task wants to switch oled on, the oled signal will be set
    xSemaphoreTake(SemaOledSignal, portMAX_DELAY);  // signal oled task to switch display on
    oledsignal = REBOOT;
    xSemaphoreGive(SemaOledSignal);

    delay(7000);
    ESP.restart();
  }

  // wifi ist ok, nun Lokale Zeit holen (NTP)

  configTime(0, 0, config.NTPPool);
  setenv("TZ", config.Timezone_Info, 1);

  bool rett = false;

  xSemaphoreTake(SemaOledSignal, portMAX_DELAY);  // signal oled task to switch display on
  oledsignal = ZEIT;                              // oled task should display the text ZEIT
  xSemaphoreGive(SemaOledSignal);

// we need to wait until NPT Time is available
  while (!rett) {
    rett = getLocalTime(&timestrukt, 15);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    //  Serial.println("getLocalTime() returns false");
  }
// finally we have everything togehter: wifi is ok, time is valid
// let's roll....
  xSemaphoreTake(SemaOledSignal, portMAX_DELAY);  // signal oled task to switch display on
  oledsignal = NORMAL;                              // oled task should display the text ZEIT
  xSemaphoreGive(SemaOledSignal);


  time(&aktuelle_epochzeit);  // get epoch time for the first time..... this needs to be here

  if (debug_flag) {  // print some time Info if debug
    Serial.println(asctime(&timestrukt));
    strftime(buffer1, sizeof(buffer1), "%d/%m/%Y %H:%M:%S", &timestrukt);
    Serial.println(buffer1);
    Serial.println(aktuelle_epochzeit);
  }

// time stuff 
  getTimeStamp();
  getCurrTime(false);
  sprintf(time_lastreset, "%s", currTime_string);
  if (essential_debug) {
    Serial.print("Boot Time: ");
    Serial.println(time_lastreset);
  }

  display_control = true;             // now that time is valid, we can enable correct display OFF
  DEBUGPRINT1("Setup done, Dauer (ms): ");
        DEBUGPRINTLN1(millis() - previousMillis);

  // create other tasks ------------

  if (essential_debug) Serial.println("WiFi OK, Local Time OK, start other tasks");
  //  task can only be started after time is available...
  vTaskDelay(100 / portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(task_clock, "clock", 5000, NULL, TASK_PRIORITY, &Task2, CORE_1);

  vTaskDelay(200 / portTICK_PERIOD_MS);

  xTaskCreatePinnedToCore(task_detect, "Movement", 2000, NULL, TASK_PRIORITY, &Task3, CORE_1);

  vTaskDelay(200 / portTICK_PERIOD_MS);

  // Tast state machine  - the main loop
  xTaskCreatePinnedToCore(state_machine, "STM", 10000, NULL, TASK_PRIORITY, &Task4, CORE_1);

  vTaskDelay(200 / portTICK_PERIOD_MS);

  // end creating tasks ------------------

  // do a log entry
  // currTime_string already set
  logMessage = String(currTime_string) + "," + "Setup ended" + "\r\n";
  log_SDCard(logMessage, path);

  if (essential_debug) Serial.println(logMessage);

  vTaskDelay(700 / portTICK_PERIOD_MS);
  vTaskDelete(NULL);  // delete this initial setup task

  // done with setup - this task is no longer needed
}


//----------------------------------------------------
//  Loop, runs on core 1 by default -------------------
void loop() {
  vTaskDelay(400 / portTICK_PERIOD_MS);
}
//--------------------------------------------
// end of code
//--------------------------------------------
//
