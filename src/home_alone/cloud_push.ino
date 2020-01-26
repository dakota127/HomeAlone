/*
* Home Alone Application
* Based on a project presentd by Ralph Bacon

* This version based on a state machine
* 


*/
// Pushover
#include "Pushover.h"
String Token  = "ajiu7a1odbcx9xp6op2wenoctneeik";
String Userkey   = "uku7m4n8ru7rwf1wkmxvhbqniq4bxh";


//--------------------------------------------------
//  Thingsspeak Stuff  
//-------------------------------------------------
int report_toCloud(int count) {
  
// NOte:
// For users of the free option, the message update interval limit remains limited at 15 seconds.
   DEBUGPRINT1 ("Cloud Update count: ");
   DEBUGPRINTLN1 (count);

   int ret = setup_wifi (WIFI_DETAILS);
   if (ret>5) {
      Serial.println ("error-error-error - no wifi 2"); 
      credential_to_use = 0;
      DEBUGPRINTLN1 ("could not report to cloud");
      return (9);
    }
 

 // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
  // pieces of information in a channel.  Here, we write to field 1.
  //  ThingSpeak.begin(client);  // Initialize ThingSpeak
  int x = ThingSpeak.writeField(config.ThingSpeakChannelNo , 1, count, (char *)config.ThingSpeakWriteAPIKey);
  if(x == 200){
    DEBUGPRINTLN1 ("Channel update successful.");

      value3_oled =1;
      xSemaphoreTake(SemaOledSignal, portMAX_DELAY);    // signal oled task to switch display on
      oledsignal = 1;
      xSemaphoreGive(SemaOledSignal);
     wifi_disconnect();
     return (0);
  }
  else{
    DEBUGPRINTLN0 ("Problem updating channel. HTTP error code " + String(x));
    return(x);
  }

 }    // end report to cloud


// -------------------------------------------------
// Pushover  stuff 
//----------------------------------------------------

int report_toPushover (String messageText) {

   DEBUGPRINT1 ("Send Pushover Message: ");
   DEBUGPRINTLN1 (messageText);

  
  Pushover po = Pushover(Token,Userkey);
  po.setDevice("Device1");
  po.setMessage(messageText);
  po.setSound("bike");
  ret = po.send();
  DEBUGPRINT1 ("returcode Pushover: "); 
  DEBUGPRINTLN1 (ret); //should return 1 on success
  
  return (ret);
}


//  end of code -------------------------------
 
