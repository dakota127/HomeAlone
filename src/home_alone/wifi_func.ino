/* -------------------------------------------------------------------
  Home Alone Application
  Based on a project presentd by Ralph Bacon
  This version used an ESP32 doing multitasking.


  //---------------------------------------------------------------
  // conncetion issues see
  // https://github.com/espressif/arduino-esp32/issues/1212
  //
  //---------------------------------------------------------------

*/
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>
#include "home_alone.h"


char wochentage[7][12] = { "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag" };
char monat[12][12] = { "Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember" };


const char* apiToken = "ajiu7a1odbcx9xp6op2wenoctneeik";
const char* userToken = "uku7m4n8ru7rwf1wkmxvhbqniq4bxh";

//Pushover API endpoint
const char* pushoverApiEndpoint = "https://api.pushover.net/1/messages.json";

// Create a WiFiClientSecure object
WiFiClientSecure secure_client;

// Create an HTTPClient object
HTTPClient https;

volatile int reason_disconnect;
volatile bool wifi_status = false;
bool wifi_ret = false;
unsigned long waitbeginMillis;

time_t aktuelle_zeit;
tm tm;
char buffer[80];

//Pushover root certificate (valid from 11/10/2006 to 15/01/2038)
const char* PUSHOVER_ROOT_CA = "-----BEGIN CERTIFICATE-----\n"
                               "MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n"
                               "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
                               "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
                               "MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n"
                               "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
                               "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n"
                               "9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n"
                               "2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n"
                               "1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n"
                               "q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n"
                               "tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n"
                               "vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n"
                               "BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n"
                               "5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n"
                               "1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n"
                               "NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n"
                               "Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n"
                               "8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n"
                               "pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n"
                               "MrY=\n"
                               "-----END CERTIFICATE-----\n";



// Zeitstruktur, wird abgefüllt durch Funktion wifi_getTime()
typedef struct {
  bool debug;
  int jahr;
  char monat[12];
  int monat_zahl;
  int tag_zahl;
  char tag[12];
  int stunde;
  int minute;
  int sekunde;
  int somwint;
  time_t epochtime;
} zeit_struct2;

int order;
int ret_int;  // retcode 0 all is well, >0 = not ok
int ret_wifi;
bool ret_bool;
bool wifi_status_1 = false;
bool debug = true;

// Definition Zeit Struktur
// wird von der Funktion wifi_getTime() gefüllt
zeit_struct2 zeit_data;  // This is a global variable


//----------------------------------------------------------------
// Callback Functions WiFi Events ---------------
void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {

  Serial.println(F("\nConnected to AP successfully!"));

  wifi_status = true;
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  wifi_status = false;
  Serial.println(F("\t\tWiFi lost connection. Reason: "));
  Serial.println(info.wifi_sta_disconnected.reason);

  reason_disconnect = info.wifi_sta_disconnected.reason;
  //if (reason_disconnect==201) Serial.println ("\t\tSSID nicht gefunden");
  //
  Serial.println(F("\t\ttrying to reconnect"));

  WiFi.reconnect();
}

//----------------------------------------------------------------
// Function _wifi_wait() -------------------------------------
bool wifi_wait(int waitTime_toConnect) {

  Serial.print(F("_wifi_wait() called: "));
  Serial.println(waitTime_toConnect);
  waitbeginMillis = millis();  // note millis, might be used later...

  // wir warten bis WiFi Connection zustande kommt
  // WiFi Event WiFiStationConnected meldet dies, siehe weiter oben
  while (true) {
    delay(100);  // auf ESP32 wird dies zu vTaskDelay(100 / portTICK_PERIOD_MS);
    //Serial.println ("\t\twaiting");
    if (wifi_status) return (true);  // OK Connection vorhanden

    // Pause the task for n ms
    delay(100);  // auf ESP32 wird dies zu vTaskDelay(100 / portTICK_PERIOD_MS);
    // stop if timeout reached
    if (millis() - waitbeginMillis > waitTime_toConnect) {
      Serial.println(F("\t\tno connection within timeout"));

      return (false);
    }
  }
}


// Function Call back (WiFi Funktion)
//-------------------------------------------------------
void callback(int status) {

  Serial.print("callback wurde aufgerufen, Status: ");
  Serial.println(status);
}

//  wifi function ---------------------------------------
//--------------------------------------------------------
int wifi_func() {

  // what is there to do ??
  switch (wifi_order_struct.order) {

    //------------------
    case DO_WIFI:
      DEBUGPRINTLN3("\t\t\t\t\twifi_func: setup wifi");

      Serial.println(credentials[0]);
      Serial.println(credentials[1]);

      WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);

      delay(100);  // für ESP32 wird dies zu vTaskDelay(100 / portTICK_PERIOD_MS);

    
                                  //----------------------------Connect to WLAN ---------------------------

      WiFi.mode(WIFI_STA);  // Wifi mode is station
      WiFi.begin(credentials[0], credentials[1]);
      //  WiFi.begin(credentials[0], "aaa");            // Test only !!
      ret_bool = wifi_wait(10000);  // try for 10 seconds

      if (ret_bool) {
        Serial.println("Connected to WiFi");
        value2_oled = 1;
        
        delay(200);  // für ESP32 wird dies zu vTaskDelay(200 / portTICK_PERIOD_MS);
                     // dies machen wir erst jetzt...
        WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);



        // getLocalTime()   Liest die aktuelle Zeit
        // This function will populate the struct with the time information, which we will access below.
        // As output, this function returns a Boolean indicating if it was
        //possible to retrieve the time information (true) or not (false). We will use the returning value for error checking.
        // https://techtutorialsx.com/2021/09/01/esp32-system-time-and-sntp/
        // setze 5 sekunden delay, damit Zeit/Jahr richtig geholt wird.



        /*
        // verschiedene Arten, die Zeit auszugeben......
        Serial.println(asctime(&tm));
        strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &tm);
        Serial.println(buffer);
*/

        return (0);
      } else {
        return (9);
      }

      break;

    //------------------
    case THINGSPEAK:
      DEBUGPRINTLN1("\t\t\t\t\twifi_func: report thingspeak");

      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\t\t\t\t\terror-error-error - no wifi 2");
        value2_oled = 7;
      } else {  // wifi ok
        value2_oled = 1;
        ret_int = report_Thingspeak (wifi_order_struct.mvcount);  // report to thingspeak
        DEBUGPRINT1("report_Thingspeak: ");
        DEBUGPRINTLN1(ret_int);  // 0 = ok, 9 = error
        if (ret_int > 0) {
          value2_oled = 5;
          getTimeStamp();
          getCurrTime(false);
          logMessage = String(currTime_string) + ",err thingsspeak\r\n";
          log_SDCard(logMessage, path);
        }
        // if not true: was error
      }
      break;

    //------------------
    case PUSHOVER:
      
      if (essential_debug) {
        Serial.print ("\t\t\t\t\twifi_func: push message: ");
        Serial.println (wifi_order_struct.pushtext);
      }
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\t\t\t\t\terror-error-error - no wifi 3");
        value3_oled = 7;
        getTimeStamp();
        getCurrTime(false);
        logMessage = String(currTime_string) + ",no wifi 3\r\n";
        log_SDCard(logMessage, path);

      } else {
        value3_oled = 1;                                                                      // default if push ok
        ret_int = 0;                                                                          // default ok
        ret_int = report_toPushover(wifi_order_struct.pushtext, wifi_order_struct.priority);  // returns true if ok
        // ok returncode is true
        if (essential_debug) {
        Serial.print ("\t\t\t\t\treport_to_push returns:: ");
        Serial.println(ret_bool);
        }

        if (ret_int > 0) {
          value3_oled = 8;  // if not true: was error
          ret_int = 1;
        }
      }
      break;

    default:

      DEBUGPRINTLN1("\t\t\t\t\tERROR Taskwifi wrong order");
      ret_int = 3;

  }  // end case statement -----------------------


  xSemaphoreTake(SemaOledSignal, portMAX_DELAY);  // signal oled task to switch display on
  oledsignal = NORMAL;
  xSemaphoreGive(SemaOledSignal);



  return (ret_int);  // wifi_func returns.....


}  // end function




//get_time();

//--------------------------------------------------------------
int report_toPushover(String messageText, int prio) {

  //
  // -----------------------------------------------------------------------------------------
  DEBUGPRINT1("report_toPushover: ");
  DEBUGPRINT1(messageText);
  DEBUGPRINT1("  prio: ");
  DEBUGPRINTLN1(prio);
  DEBUGPRINT1("devices: ");
  DEBUGPRINTLN1(config.PushoverDevices);
  DEBUGPRINT1("token: ");
  DEBUGPRINT1(config.PushoverToken);
  DEBUGPRINT1("  Key: ");
  DEBUGPRINTLN1(config.PushoverUserkey);

  getTimeStamp();
  getCurrTime(false);
  logMessage = String(currTime_string) + ",Trying pushover\r\n";
  log_SDCard(logMessage, path);


  tokenstring = config.PushoverToken;
  userkeystring = config.PushoverUserkey;
  devicestring = config.PushoverDevices;
  personstring = config.PersonName;



  // Create a JSON object with notification details
  // Check the API parameters: https://pushover.net/api
  StaticJsonDocument<512> notification;
  notification["token"] = apiToken;              //required
  notification["user"] = userToken;              //required
  notification["message"] = messageText;         //required
  notification["title"] = "ESP32 Notification";  //optional
  notification["device"] = "petersiphone";       //optional
  notification["url"] = "";                      //optional
  notification["url_title"] = "";                //optional
  notification["html"] = "";                     //optional
  notification["priority"] = "";                 //optional
  notification["sound"] = "bike";                //optional
  notification["timestamp"] = "";                //optional

  // Serialize the JSON object to a string
  String jsonStringNotification;
  serializeJson(notification, jsonStringNotification);
  if (essential_debug) {
    Serial.println("Send this to Pushover: ");
    Serial.println(jsonStringNotification);
  }

  // Set the certificate for the WiFiClientSecure object
  secure_client.setCACert(PUSHOVER_ROOT_CA);
  // Specify the target URL
  https.begin(secure_client, pushoverApiEndpoint);

  // Add headers
  https.addHeader("Content-Type", "application/json");

  // Send the POST request with the JSON data
  int httpResponseCode = https.POST(jsonStringNotification);


  // Check the response
  if (httpResponseCode > 0) {
    Serial.printf("HTTP response code: %d\n", httpResponseCode);
    /*
    String response = https.getString();
    Serial.println("Response:");
    Serial.println(response);
    */
  } else {
    Serial.printf("ERROR HTTP response code: %d\n", httpResponseCode);
    Serial.println(https.errorToString(httpResponseCode));
    // Close the connection
    https.end();
  }

   
  if (httpResponseCode == 200) {
    if (essential_debug) Serial.println("Pushover was OK");
    return (0);
  } else {
    if (essential_debug) Serial.println("Pushover FAILED");
    return (9);
  }
}




//--------------------------------------------------
void wifi_details() {
  DEBUGPRINTLN3("");
  DEBUGPRINT3("WiFi connected to SSID: ");
  DEBUGPRINTLN3(WiFi.SSID());
  DEBUGPRINT3("IP address: ");
  DEBUGPRINTLN3(WiFi.localIP());
  DEBUGPRINTLN3("\nWiFi Details:");
  WiFi.printDiag(Serial);
  DEBUGPRINTLN3("");
}





//--------------------------------------------------------------------------------------------------



// -------------------------------------------------

void getCurrTime(bool how) {

  delay(200);

  if (how) strftime(currTime_string, 30, "%a  %d-%m-%y %T", localtime(&aktuelle_epochzeit));
  else strftime(currTime_string, 30, "%d-%m-%y %H:%M:%S", localtime(&aktuelle_epochzeit));
  DEBUGPRINTLN3(currTime_string);
}

//----------------------------------------------
// Function to get date and time from NTPClient
void getTimeStamp() {

  DEBUGPRINTLN2("->doing getTimeStamp()");
  //get_time();
  // Extract time
  timeStamp = aktuelle_epochzeit;
  DEBUGPRINT3("timeStamp: ");
  DEBUGPRINTLN3(timeStamp);
}


// -------------------------------------------------
/*
void showTime (tm localTime) {
  Serial.print(localTime.tm_mday);
  Serial.print('.');
  Serial.print(localTime.tm_mon + 1);
  Serial.print('.');
  Serial.print(localTime.tm_year - 100);
  Serial.print('-');
  Serial.print(localTime.tm_hour);
  Serial.print(':');
  Serial.print(localTime.tm_min);
  Serial.print(':');
  Serial.print(localTime.tm_sec);
  Serial.print(" Wochentag ");

  if (localTime.tm_wday == 0) Serial.println(7);
  else Serial.println(localTime.tm_wday);
}
*/

// -------------------------------------------------
//------------------------------------------
// this not currently used -----
// ---------------------------------------------------
void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.print("Local time: ");
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

//---------------------------------------------------
// end of code --------------------------------------------
//---------------------------------------------------
