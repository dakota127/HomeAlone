/* -------------------------------------------------------------------
* Home Alone Application
* Based on a project presentd by Ralph Bacon
* This version used an ESP32 doing multitasking.
* by Peter B, from Switzerland
* Project website http://projects.descan.com/projekt7.html
* 

* JSON:
* arduinojson.org/v6/assistant 
* to compute the capacity.
*/


#define NUMBEROF_TRIES 2

const char*  push_server = "api.pushover.net";  // Server URL
// ThingSpeak settings
const char* thing_server = "api.thingspeak.com";
String tokenstring;
String userkeystring;
String devicestring;
String personstring;
String line;
String linelog;
int timeout_at;
//--------------------------------------------------
//  Thingsspeak Stuff  
//-------------------------------------------------
int report_toCloud (int count) {
  
// NOte:
// For users of the free option, the message update interval limit remains limited at 15 seconds.
   DEBUGPRINT1 ("Cloud Update count: ");
   DEBUGPRINTLN1 (count);


 // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
  // pieces of information in a channel.  Here, we write to field 1.
  ThingSpeak.begin(client_thing);  // Initialize ThingSpeak

  int x = ThingSpeak.writeField (config.ThingSpeakChannelNo , 1, count, (char *)config.ThingSpeakWriteAPIKey);
  if(x == 200) {
    DEBUGPRINTLN1 ("Channel update successful.");
    return (0);
  }
  else {
    DEBUGPRINTLN0 ("Problem updating channel. HTTP error code " + String(x));
    return(9);
  }

 }    // end report to cloud

 
// -------------------------------------------------
// Pushover  stuff 
//----------------------------------------------------
//--------------------------------------------------------------
int push_msg (String text, int prio){
  
// ------- report to pushover ----------------------------------

 //   return(0);

    DEBUGPRINTLN1 ("\t\tsend Pushover message");

         // set up parameter for this job
        wifi_todo = PUSH_MESG;
        wifi_order_struct.order = wifi_todo;
        wifi_order_struct.pushtext = text;
        wifi_order_struct.priority = prio;    
        ret = wifi_func();
        DEBUGPRINT2 ("\t\twifi_func returns: ");   DEBUGPRINTLN2 (ret);
        if (ret == 0) {        
         DEBUGPRINTLN1 ("\t\tmessage sent ok");
        }
        xSemaphoreTake(SemaOledSignal, portMAX_DELAY);    // signal oled task to switch display on
         oledsignal = 1;
        xSemaphoreGive(SemaOledSignal);        
        
        vTaskDelay(200 / portTICK_PERIOD_MS);
        return (ret); 
// ------- report to pushover ----------------------------------
     
}

//--------------------------------------------------------------
bool report_toPushover (String messageText, int prio) {

// Attention !!
// this is not done using the pushover library because I wanted to use better errorhandling.
// -----------------------------------------------------------------------------------------

  DEBUGPRINT1 ("report_toPushover: "); DEBUGPRINT1 (messageText);  DEBUGPRINT1 ("  prio: "); DEBUGPRINTLN1 (prio);
  DEBUGPRINT1 ("devices: "); DEBUGPRINTLN1 (config.PushoverDevices);
  DEBUGPRINT1 ("token: "); DEBUGPRINT1 (config.PushoverToken); DEBUGPRINT1 ("  Key: "); DEBUGPRINTLN1 (config.PushoverUserkey);


  tokenstring = config.PushoverToken;
  userkeystring = config.PushoverUserkey;
  devicestring = config.PushoverDevices;
  personstring = config.PersonName;


  DEBUGPRINTLN2 ("\nStarting connection to pushover server...");
  
  Pushover po = Pushover(config.PushoverToken,config.PushoverUserkey);
  po.setHTML (true);
  po.setDevice("myiphone,myipad");
  po.setTitle (personstring);
  po.setMessage(messageText);
  po.setPriority(prio);  
  po.setSound("bike");
  bool ret = po.send();
  DEBUGPRINT2 ("Returncode: "); DEBUGPRINTLN2 (ret);     //should return true on success 
 
  if (!ret) {
    getTimeStamp();
    getCurrTime(false); 
    logMessage = String(currTime) + "," + "pushover failed" + "\r\n";
    log_SDCard(logMessage, path);   
  }

  return (ret);
 
}



//-------------------------------------------------------
void store_reset_reason(RESET_REASON reason)
{
 
  switch ( reason)
  {

    case 1 :  sprintf( reset_reason , "%s ", "POWERON_RESET") ;   break;       /**<1, Vbat power on reset*/
    case 3 :  sprintf( reset_reason , "%s ", "SW_RESET");         break;       /**<3, Software reset digital core*/
    case 4 :  sprintf( reset_reason , "%s ", "OWDT_RESET");       break;       /**<4, Legacy watch dog reset digital core*/
    case 5 :  sprintf( reset_reason , "%s ", "DEEPSLEEP_RESET");  break;       /**<5, Deep Sleep reset digital core*/
    case 6 :  sprintf( reset_reason , "%s ", "SDIO_RESET");       break;       /**<6, Reset by SLC module, reset digital core*/
    case 7 :  sprintf( reset_reason , "%s ", "TG0WDT_SYS_RESET"); break;       /**<7, Timer Group0 Watch dog reset digital core*/
    case 8 :  sprintf( reset_reason , "%s ", "TG1WDT_SYS_RESET"); break;       /**<8, Timer Group1 Watch dog reset digital core*/
    case 9 :  sprintf( reset_reason , "%s ", "RTCWDT_SYS_RESET"); break;       /**<9, RTC Watch dog Reset digital core*/
    case 10 : sprintf( reset_reason , "%s ", "INTRUSION_RESET");  break;       /**<10, Instrusion tested to reset CPU*/
    case 11 : sprintf( reset_reason , "%s ", "TGWDT_CPU_RESET");  break;       /**<11, Time Group reset CPU*/
    case 12 : sprintf( reset_reason , "%s ", "SW_CPU_RESET");     break;       /**<12, Software reset CPU*/
    case 13 : sprintf( reset_reason , "%s ", "RTCWDT_CPU_RESET"); break;       /**<13, RTC Watch dog Reset CPU*/
    case 14 : sprintf( reset_reason , "%s ", "EXT_CPU_RESET");    break;       /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : sprintf( reset_reason , "%s ", "RTCWDT_BROWN_OUT_RESET");break;  /**<15, Reset when the vdd voltage is not stable*/
    case 16 : sprintf( reset_reason , "%s ", "RTCWDT_RTC_RESET"); break;       /**<16, RTC Watch dog reset digital core and rtc module*/
    default : sprintf( reset_reason , "%s ", "NO_MEAN");
  }
  
}



// end of code ------------------------------
