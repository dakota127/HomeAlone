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
        if (ret == 0) {           // reset count if ok 
         DEBUGPRINTLN1 ("\t\tmessage sent ok");
        }
        xSemaphoreTake(SemaOledSignal, portMAX_DELAY);    // signal oled task to switch display on
         oledsignal = 1;
        xSemaphoreGive(SemaOledSignal);        
        
        vTaskDelay(200 / portTICK_PERIOD_MS);
        return(ret); 
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
  
  if (!client_push.connect(push_server, 443)) {
    DEBUGPRINTLN2 ("Connection failed!");
    return (false);
  }
    DEBUGPRINTLN2 ("Connected to pushover server");


    vTaskDelay(300 / portTICK_PERIOD_MS); 

 //   int lendata = String(config.PushoverToken).length() + String(config.PushoverUserkey).length() + String(config.PushoverDevices).length() + String(config.PersonName).length() ;  
     int lendata = tokenstring.length() + userkeystring.length() + devicestring.length() + personstring.length() ;  
    
    DEBUGPRINT2 ("Länge daten: ");      DEBUGPRINTLN2 (lendata);
    DEBUGPRINT2 ("Länge message: ");   DEBUGPRINTLN2 (messageText.length());
    lendata = lendata + 46 + messageText.length();
 //   Serial.print ("Länge daten total: "); Serial.println (lendata);
    
    // Make a HTTP request:
    client_push.println ("POST /1/messages.json HTTP/1.1");
    client_push.println ("Host: api.pushover.net");
    client_push.println ("Content-Type: application/x-www-form-urlencoded");
    client_push.println ("Connection: close");
    client_push.print  ("Content-Length: ");
    client_push.print  (lendata);
    client_push.println("\r\n");


//  //   46 literals plus 76 daten  = 112
    client_push.print("token=" + tokenstring + "&user=" + userkeystring + "&device=" + devicestring + "&title=" + personstring + "&priority=" + prio + "&message=" + messageText);

    timeout_at = millis() + 6000;
    while (client_push.connected()) {
       DEBUGPRINTLN3 ("im loop");
       if (timeout_at - millis() < 0) {
             DEBUGPRINTLN1 ("timeout_push, abort sending");
             return(false);
       }
       vTaskDelay(100 / portTICK_PERIOD_MS); 
      line = client_push.readStringUntil('\n');
      if (line == "\r") {
        DEBUGPRINTLN3 ("headers received from server");
        break;
      }
    }

 while (client_push.available() != 0) {
    if (client_push.read() == '{') break;
  }

  line = client_push.readStringUntil('\n');
  DEBUGPRINTLN2  (line);  
  if ( line.indexOf("\"status\":1") == -1) {
    DEBUGPRINTLN1 ("Pushsend bringt error!: "); 
    return (false);
  }
  else return (true);
    

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
