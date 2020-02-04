/* -------------------------------------------------------------------
* Home Alone Application
* Based on a project presentd by Ralph Bacon
* This version used an ESP32 doing multitasking.
* by Peter B, from Switzerland
* Project website http://projects.descan.com/projekt7.html
* 
//---------------------------------------------------------------
// this function runs as a separate task - it handles wifi if so requested
// it does its job then suspends itself 
// if takes the wifi_semaphore semaphore while working
//---------------------------------------------------------------

*/
int order;
int ret_code;


void task_wifi ( void * parameter )
{

 
 static bool wifi_firsttime = true ;

  for (;;) {                          // runs until doomsday 
    
    if (wifi_firsttime) {
      DEBUGPRINT1 ("\t\t\t\t\tTASK wifi_task - Running on core:");
      DEBUGPRINTLN1 (xPortGetCoreID());
      wifi_firsttime = false;
     // nothing else to do ....
    }

// ------------- suspend the wifi task until some other task wakes it up with TaskResume() -------
//------------------------------------------------------------------------------------------------
    DEBUGPRINTLN1 ("\t\t\t\t\twifi_task suspend");
    vTaskSuspend( NULL );
    DEBUGPRINTLN1 ("\t\t\t\t\twifi_task resume");

//  somebody has issued a resume and that means there is work to do..... ------------------------
//-----------------------------------------------------------------------------------------------

    vTaskDelay(500 / portTICK_PERIOD_MS);           // there is no rush...

  //   xSemaphoreTake(wifi_semaphore, portMAX_DELAY);
   

// what is there to do ??
    switch (wifi_order_struct.order) {

//------------------      
      case TEST_WIFI:
        DEBUGPRINTLN1 ("\t\t\t\t\twifi_task doing setup wifi");
        ret_code = setup_wifi ( WIFI_DETAILS);      // Test wifi connection, 
        if (ret_code > 5) {
            Serial.println("\t\t\t\t\terror-error-error - no wifi 1"); 
            value1_oled = 7;   
        }
        else {
        value1_oled = 1;   
       }

       break;

//------------------
      case REPORT_CLOUD:
        DEBUGPRINTLN1 ("\t\t\t\t\twifi_task doing report thingspeak");

        ret_code = setup_wifi ( WIFI_DETAILS);      // Test wifi connection, 
        if (ret_code > 5) {
          Serial.println("\t\t\t\t\terror-error-error - no wifi 2"); 
          value2_oled = 7;   
        }
        else {              // wifi ok
          value2_oled = 1;   
          ret_code = report_toCloud(wifi_order_struct.mvcount)  ; 
           if (ret_code > 0 ) value2_oled = 5;   
        }        
          break;

//------------------
      case PUSH_MESG:
        DEBUGPRINT1 ("\t\t\t\t\twifi_task doing push message: ");   DEBUGPRINTLN1 ( wifi_order_struct.pushtext);
        ret_code = setup_wifi ( WIFI_DETAILS);      // Test wifi connection, 
        if (ret_code > 5) {
          Serial.println("\t\t\t\t\terror-error-error - no wifi 3"); 
          value3_oled = 7;   
        }
        else {
          value3_oled = 1;   
          int ret_code = report_toPushover (wifi_order_struct.pushtext, wifi_order_struct.priority );  
 
        }
      break;

      default:

        DEBUGPRINTLN1 ("\t\t\t\t\tERROR Taskwifi wrong order");
      
    }       // end case statement -----------------------


     xSemaphoreTake(SemaOledSignal, portMAX_DELAY);    // signal oled task to switch display on
      oledsignal = 1;
     xSemaphoreGive(SemaOledSignal);

    wifi_disconnect();                // disconnect unti further use
    
    DEBUGPRINTLN1 ("\t\t\t\t\twifi_task give semaphore");
    xSemaphoreGive(wifi_semaphore);                           // release wifi semaphore
    vTaskDelay(100 / portTICK_PERIOD_MS); 
  }
  
}  // end function



// ----------------------------------------
// enable WiFi connection - with returncode
// ----------------------------------------
// return a struct wifi_t containing returncode and credentialnumer that was used if successful
//
int setup_wifi(int detail) {
  int anz_try = 0;
  bool connection = false;
  bool timeout = false;
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
      DEBUGPRINT1 ("\t\t\t\t\tTry to connect1:");  DEBUGPRINT1 (credentials[(2*i)]); DEBUGPRINT1 ("/"); DEBUGPRINTLN1 (credentials[(2*i)+1]); 
      timeout = false;

      ret = wifi_connect (credentials[(2*i)], credentials[(2*i)+1]);
      if (ret == 0) {
        connection = true;
        use_all_credentials = false;
        use_this_cred = i;
      }
      else i=i+1;                   // try next credentials
    } 
         
  }
  
  // use the credentials that were used at the begin of the sketch
  else {
    DEBUGPRINT1 ("\t\t\t\t\tTry to connect2:");  DEBUGPRINT1 (credentials[(2*use_this_cred)]); DEBUGPRINT1 ("/"); DEBUGPRINTLN1 (credentials[(2*use_this_cred)+1]); ; 
 
    ret = wifi_connect(credentials[(2*use_this_cred)], credentials[(2*use_this_cred)+1]);

    if (ret == 0) connection = true;
       
  }

   if (connection)   {
   
      DEBUGPRINT1 ("\t\t\t\t\tConnection ok  ");
      DEBUGPRINTLN1 (use_this_cred);
 
      get_time();
      if (detail >0) wifi_details();        // wifi details if needed
      return (0);                         // return retcode and credentialnumber that we used
    }

    else  {               

      return (9);                     // return retcode and credentialnumber
    }
}
     
    
//--------------------------------------------------
int wifi_connect( char *ssid, char *pw) {
  bool timeout = false;
  int anz_try = 0;
  WiFi.begin(ssid, pw);
  
  vTaskDelay(300 / portTICK_PERIOD_MS); 
  
  while (WiFi.status() != WL_CONNECTED)  { 
    anz_try++;
    if (anz_try > 45)   {// wenn noch Eintrag im Router, dann dauert es
        DEBUGPRINTLN1 ("\t\t\t\t\tno connection");
        timeout= true;
        anz_try = 0;
        break; 
    }
    vTaskDelay(300 / portTICK_PERIOD_MS); 
    DEBUGPRINT2 ("."); 
  }
  if (!timeout) return (0);
  else return(9); 
}


//--------------------------------------------------
void wifi_details()
{
   DEBUGPRINTLN1    ("");
   DEBUGPRINT1    ("WiFi connected to SSID: ");
   DEBUGPRINTLN1    (WiFi.SSID());
   DEBUGPRINT1    ("IP address: ");
   DEBUGPRINTLN1    (WiFi.localIP());
   DEBUGPRINTLN1    ("\nWiFi Details:");
   WiFi.printDiag(Serial);
   DEBUGPRINTLN1    ("");
}


//--------------------------------------------------
void wifi_disconnect () {

    //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  DEBUGPRINTLN1 ("\t\t\t\t\tWifi disconnected");

}

//-----------------------------------------
int get_time() {    
     //init and get the time
     
 configTime(0, 0, config.NTPPool);
  // See https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv for Timezone codes for your region
  setenv("TZ", config.Timezone_Info, 1);
  DEBUGPRINTLN1 (config.Timezone_Info);

     if (getNTPtime(10)) {  // wait up to 10sec to sync
        } 
      else {
          DEBUGPRINTLN0 ("Time not set");
   // ESP.restart();
        }
    if (debug_flag)  showTime(timeinfo); 
    lastNTPtime = time(&now);
    lastEntryTime = millis();

}


 
//----------------------------------------------------

bool getNTPtime(int sec) {

  DEBUGPRINTLN1 ("get time");

  {
    uint32_t start = millis();
    do {
      time(&now);
      localtime_r(&now, &timeinfo);
      DEBUGPRINT1 (".");
      delay(100);
    } while (((millis() - start) <= (1000 * sec)) && (timeinfo.tm_year < (2016 - 1900)));
    
    if (timeinfo.tm_year <= (2016 - 1900)) return false;  // the NTP call was not successful
    DEBUGPRINT1 ("now ");  DEBUGPRINTLN1 (now);
    char time_output[30];
    strftime(time_output, 30, "%a  %d-%m-%y %T", localtime(&now));
    if (debug_flag) {
      Serial.println(time_output);
      Serial.println();
    }

                                            
  return true;
}

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
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}
//    end of code --------------------------------------------


//---------------------------------------------------
