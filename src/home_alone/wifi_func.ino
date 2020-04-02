/* -------------------------------------------------------------------
* Home Alone Application
* Based on a project presentd by Ralph Bacon
* This version used an ESP32 doing multitasking.

* 
//---------------------------------------------------------------
// conncetion issues see
// https://github.com/espressif/arduino-esp32/issues/1212
//
//---------------------------------------------------------------

*/

int order;
int ret_int;       // retcode 0 all is well, >0 = not ok 
bool ret_bool;

//  wifi function ---------------------------------------
//--------------------------------------------------------
int wifi_func ()
{

// what is there to do ??
    switch (wifi_order_struct.order) {

//------------------      
      case TEST_WIFI:
        DEBUGPRINTLN3 ("\t\t\t\t\twifi_func: setup wifi");
        ret_int = setup_wifi ( WIFI_DETAILS);      // Test wifi connection, 
        if (ret_int > 5) {
            Serial.println ("\t\t\t\t\terror-error-error - no wifi 1"); 
            value1_oled = 7;   
        }
        else {
        value1_oled = 1;  
        Serial.println ("\t\t\t\t\tInitial wifi connection ok");  // ALWAYS PRINT THIS 
       }

       break;

//------------------
      case REPORT_CLOUD:
        DEBUGPRINTLN1 ("\t\t\t\t\twifi_func: report thingspeak");

        ret_int = setup_wifi ( WIFI_DETAILS);      // Test wifi connection, 
        if (ret_int > 5) {
          Serial.println("\t\t\t\t\terror-error-error - no wifi 2"); 
          value2_oled = 7;   
        }
        else {              // wifi ok
          value2_oled = 1;   
          ret_int = report_toCloud (wifi_order_struct.mvcount)  ;    // report to thingspeak
          DEBUGPRINT1 ("report_toCloud: "); DEBUGPRINTLN1 (ret_int);  // 0 = ok, 9 = error 
          if (ret_int > 0 ) value2_oled = 5;                           // if not true: was error
        }        
          break;

//------------------
      case PUSH_MESG:
        DEBUGPRINT1 ("\t\t\t\t\twifi_func: push message: ");   DEBUGPRINTLN1 ( wifi_order_struct.pushtext);
        
        ret_int = setup_wifi ( WIFI_DETAILS);      // Test wifi connection, 
        if (ret_int > 5) {
          Serial.println("\t\t\t\t\terror-error-error - no wifi 3"); 
          value3_oled = 7;   
        }
        else {
          value3_oled = 1;          // default if push ok
          ret_int = 0;              // default
          bool ret_bool = report_toPushover (wifi_order_struct.pushtext, wifi_order_struct.priority );  // returns true if ok
          // ok returncode is true 
          DEBUGPRINT2 ("\t\t\t\t\treport_to_push returns:: ");   DEBUGPRINTLN1 ( ret_bool);
          if (!ret_bool) {
            value3_oled = 8;                       // if not true: was error
            ret_int = 1;
          }
       
        }
      break;

      default:

        DEBUGPRINTLN1 ("\t\t\t\t\tERROR Taskwifi wrong order");
        ret_int = 3;
      
    }       // end case statement -----------------------


     xSemaphoreTake(SemaOledSignal, portMAX_DELAY);    // signal oled task to switch display on
      oledsignal = 1;
     xSemaphoreGive(SemaOledSignal);

    wifi_disconnect();                // disconnect unti further use

  return (ret_int);                  // wifi_func returns.....

  
}  // end function



// ----------------------------------------
// enable WiFi connection - with returncode
// ----------------------------------------
// return a struct wifi_t containing returncode and credentialnumer that was used if successful
//
int setup_wifi(int detail) {
  int anz_try = 0;
  bool connection = false;

  int anz_credentials =  (NUMITEMS (credentials))/2;
  
  static int use_this_cred = 0;                   
  static bool wifi_firsttime= true;
  static bool use_all_credentials = true;

  if (wifi_firsttime) {
    // do nothing
    wifi_firsttime = false;
  }
  
  DEBUGPRINT1 ("\t\t\t\t\tsetup_wifi: "); DEBUGPRINT1 (use_this_cred); DEBUGPRINT1 (detail);  DEBUGPRINTLN1 (anz_credentials);
  int i = 0;

//  Check all of the available wifi credentials 
  if (use_all_credentials == true) {
  
    while ((connection == false) &  ((anz_credentials - i) >0)) {
      DEBUGPRINT2 ("\t\t\t\t\tTry to connect_1:");  DEBUGPRINT2 (credentials[(2*i)]); DEBUGPRINT2 ("/"); DEBUGPRINTLN2 (credentials[(2*i)+1]); 
  
      ret = wifi_connect (credentials[(2*i)], credentials[(2*i)+1]);
      if (ret == 0) {
        connection = true;
        use_all_credentials = false;
        use_this_cred = i;
      }
      else i=i+1;                   // try next credentials
    } 
         
  }
  
  // use the credentials that was used and worked during setup() 
  else {
    DEBUGPRINT2 ("\t\t\t\t\tTry to connect_2:");  DEBUGPRINT2 (credentials[(2*use_this_cred)]); DEBUGPRINT2 ("/"); DEBUGPRINTLN2 (credentials[(2*use_this_cred)+1]); ; 
 
    ret = wifi_connect (credentials[(2*use_this_cred)], credentials[(2*use_this_cred)+1]);

    if (ret == 0) connection = true;
       
  }

   if (connection)   {
   
      DEBUGPRINT1 ("\t\t\t\t\tConnection ok  \n");
      DEBUGPRINTLN2 (use_this_cred);
 
      get_time();
      if (detail >0) wifi_details();        // wifi details if needed
      return (0);                         // return retcode and credentialnumber that we used
    }

    else  {               

      return (9);                     // return retcode 
    }
}
     


//--------------------------------------------------
int wifi_connect( char *ssid, char *pw) {
  bool have_connection = true;
  int anz_try = 20;

//  DEBUGPRINT2 ("\t\t\t\t\t");  DEBUGPRINT2 (ssid); DEBUGPRINT2 ("/"); DEBUGPRINTLN2 (pw);

  
  WiFi.begin(ssid, pw);
  
  vTaskDelay(300 / portTICK_PERIOD_MS); 

    wifiStatus = false;
    while ((anz_try > 0) && (!wifiStatus))
    {
        wifiClear();
        WiFi.begin(ssid, pw);
        vTaskDelay(2000 / portTICK_PERIOD_MS); 
        wifiStatus = waitForWifi();
   //     wifiStatus = WiFi.status() == WL_CONNECTED;
        anz_try-- ;
    }
    Serial.println("");
    
  if (wifiStatus) return (0);
  else return(9); 
}

//------------------------------------------------
bool waitForWifi() {
//  DEBUGPRINT1 ("\n");
  DEBUGPRINT1 ("\t\t\t\t\twaiting for WiFi.");

int retries;
 
   retries = 0;
   while (WiFi.status() != WL_CONNECTED)  { 
    retries++;
    if (retries > 20) break;      // max number retries
  // Pause the task for 300ms
    vTaskDelay(300 / portTICK_PERIOD_MS);
    DEBUGPRINT2  ("."); 
   }
    
  if (WiFi.status() == WL_CONNECTED) {
    DEBUGPRINTLN3  ("\n\t\t\t\t\tconnected");
    return (true);
  }

  // no connection...
  DEBUGPRINTLN3  ("\n\t\t\t\t\tno wifi connection");
  return (false);
}

//--------------------------------------------------
void wifi_details()
{
   DEBUGPRINTLN3    ("");
   DEBUGPRINT3    ("WiFi connected to SSID: ");
   DEBUGPRINTLN3    (WiFi.SSID());
   DEBUGPRINT3    ("IP address: ");
   DEBUGPRINTLN3    (WiFi.localIP());
   DEBUGPRINTLN3    ("\nWiFi Details:");
   WiFi.printDiag(Serial);
   DEBUGPRINTLN3    ("");
}


//--------------------------------------------------
void wifi_disconnect () {
// Name : wifi_disconnect
// Purpose : disconnects from the network and switches off the radio. sets wifiStatus to false.
// Argument(s) : void
// Returns : void
//disconnect WiFi as it's no longer needed
    
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  wifiStatus = false;
  DEBUGPRINTLN2 ("\t\t\t\t\tWifi disconnected");
  vTaskDelay(300 / portTICK_PERIOD_MS); 
}


//--------------------------------------------------------------------------------------------------
// Function Details
//--------------------------------------------------------------------------------------------------
// Name : wifiClear
// Purpose : clears all wifi settings before connecting. must be run before every wifiConnect
// Argument(s) : void
// Returns : void

void wifiClear()
{
    DEBUGPRINTLN1 ("\n\t\t\t\t\twifiClear()");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    wifiStatus = WiFi.status() == WL_CONNECTED;
    delay(100);
}
//--------------------------------------------------------------------------------------------------

//-----------------------------------------
int get_time() {    
     //init and get the time
     
 configTime(0, 0, config.NTPPool);
  // See https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv for Timezone codes for your region
  setenv("TZ", config.Timezone_Info, 1);
  DEBUGPRINTLN2 (config.Timezone_Info);

     if (getNTPtime(10)) {  // wait up to 10sec to sync
        } 
      else {
          DEBUGPRINTLN0 ("Time not set <------------");
          vTaskDelay(500 / portTICK_PERIOD_MS);               // wait, to prevent fast rebbot sequence
          ESP.restart();
        }
  //  if (debug_flag)  showTime(timeinfo); 
    lastNTPtime = time(&now);
    lastEntryTime = millis();

}


 
//----------------------------------------------------

bool getNTPtime(int sec) {

  DEBUGPRINTLN2 ("get time");

    uint32_t start = millis();
    do {
      time(&now);
      localtime_r(&now, &timeinfo);
      DEBUGPRINT1 (".");
      delay(100);
    } while (((millis() - start) <= (1000 * sec)) && (timeinfo.tm_year < (2016 - 1900)));
    
    if (timeinfo.tm_year <= (2016 - 1900)) return false;  // the NTP call was not successful
    DEBUGPRINT2 ("now ");  DEBUGPRINTLN2 (now);

                                            
  return true;
}


// -------------------------------------------------

void showTime(tm localTime) {
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
  if (localTime.tm_wday == 0)   Serial.println(7);
  else Serial.println(localTime.tm_wday);
}


// -------------------------------------------------
//------------------------------------------
// this not currently used -----
// ---------------------------------------------------
void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.print ("Local time: ");
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

//---------------------------------------------------
// end of code --------------------------------------------
//---------------------------------------------------
