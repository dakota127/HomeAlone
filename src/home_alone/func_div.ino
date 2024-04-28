/* -------------------------------------------------------------------
  Home Alone Application
  Based on a project presentd by Ralph Bacon
  This version used an ESP32 doing multitasking.
  by Peter B, from Switzerland
  Project website http://projects.descan.com/projekt7.html

  Pushover Library used
  https://github.com/dakota127/myPushover

  JSON:
  arduinojson.org/v6/assistant
  to compute the capacity.
*/


#define NUMBEROF_TRIES 2

// ThingSpeak settings
const char* thing_server = "api.thingspeak.com";
String tokenstring;
String userkeystring;
String devicestring;
String personstring;
String line;
String linelog;
int timeout_at;
int retc;
String html_response;

//--------------------------------------------------
//  Thingsspeak Stuff
//-------------------------------------------------
int report_Thingspeak (int count) {

  // NOte:
  // For users of the free option, the message update interval limit remains limited at 15 seconds.
  DEBUGPRINT1 ("report_Thingspeak count: ");
  DEBUGPRINTLN1 (count);


  // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
  // pieces of information in a channel.  Here, we write to field 1.
  ThingSpeak.begin(client_thing);  // Initialize ThingSpeak

  retc  = ThingSpeak.writeField (config.ThingSpeakChannelNo , 1, count, (char *)config.ThingSpeakWriteAPIKey);
  if (retc == 200) {
    DEBUGPRINTLN1 ("Channel update successful.");
    return (0);
  }
  else {
    DEBUGPRINTLN0 ("Problem updating channel. HTTP error code " + String(retc));
    return (9);
  }

}    // end report to cloud


// -------------------------------------------------
// Pushover  stuff
//----------------------------------------------------
//--------------------------------------------------------------
int push_msg (String text, int prio) {

  // ------- report to pushover ----------------------------------

  //   return(0);

  DEBUGPRINTLN1 ("\t\tsend Pushover message");

  // set up parameter for this job
  wifi_todo = PUSHOVER;
  wifi_order_struct.order = wifi_todo;
  wifi_order_struct.pushtext = text;
  wifi_order_struct.priority = prio;
  retc = wifi_func();
  DEBUGPRINT2 ("\t\twifi_func returns: ");   DEBUGPRINTLN2 (retc);
  if (retc == 0) {
    DEBUGPRINTLN1 ("\t\tmessage sent ok");
  }
  xSemaphoreTake(SemaOledSignal, portMAX_DELAY);    // signal oled task to switch display on
  oledsignal = NORMAL;
  xSemaphoreGive(SemaOledSignal);

  vTaskDelay(200 / portTICK_PERIOD_MS);
  return (retc);
  // ------- report to pushover ----------------------------------

}






// end of code ------------------------------
