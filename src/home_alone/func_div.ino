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
// Pushover
#include "Pushover.h"


//----------------------------------------------------------------


void test_push(String message, int prio) {
  
// we need to report to the cloud - this is done in the wifi task
 // first we need to se if task is free or busy - we check the tasks semaphore

  
  
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait 10 ticks to see if it becomes free. */
        if( xSemaphoreTake( wifi_semaphore, ( TickType_t ) 100 ) == pdTRUE )
        {
            /* We were able to obtain the semaphore and can now access the
            shared resource. */
         // set up parameter for this job
            wifi_todo = PUSH_MESG;
            wifi_order_struct.order = wifi_todo;
            wifi_order_struct.pushtext = message;
            wifi_order_struct.priority = prio;             // 1 is HIGH  
            DEBUGPRINTLN2 (wifi_order_struct.pushtext);
            vTaskResume( Task1 );
            /* We have finished accessing the shared resource.  Release the
            semaphore. */
            DEBUGPRINTLN2 ("\t\t\t\t\ttest_push give semaphore");
              xSemaphoreGive(wifi_semaphore);                           // release wifi semaphore
        }
        else
        {
            /* We could not obtain the semaphore and can therefore not access
            the shared resource safely. */

            DEBUGPRINTLN1  ("wifi semaphore busy (push test msg)");
            value3_oled = 5;   
            xSemaphoreTake(SemaOledSignal, portMAX_DELAY);    // signal oled task to switch display on
              oledsignal = 1;
            xSemaphoreGive(SemaOledSignal);        
            
            vTaskDelay(200 / portTICK_PERIOD_MS);

        }
}      



//--------------------------------------------------
//  Thingsspeak Stuff  
//-------------------------------------------------
int report_toCloud(int count) {
  
// NOte:
// For users of the free option, the message update interval limit remains limited at 15 seconds.
   DEBUGPRINT1 ("Cloud Update count: ");
   DEBUGPRINTLN1 (count);

 // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
  // pieces of information in a channel.  Here, we write to field 1.
  //  ThingSpeak.begin(client);  // Initialize ThingSpeak
  int x = ThingSpeak.writeField (config.ThingSpeakChannelNo , 1, count, (char *)config.ThingSpeakWriteAPIKey);
  if(x == 200) {
    DEBUGPRINTLN1 ("Channel update successful.");
    return (0);
  }
  else {
    DEBUGPRINTLN0 ("Problem updating channel. HTTP error code " + String(x));
    return(x);
  }




 }    // end report to cloud


// -------------------------------------------------
// Pushover  stuff 
//----------------------------------------------------

int report_toPushover (String messageText, int prio) {

  DEBUGPRINT1 ("report_toPushover: "); DEBUGPRINT1 (messageText);  DEBUGPRINT1 ("  prio: "); DEBUGPRINTLN1 (prio);
  DEBUGPRINT1 ("devices: "); DEBUGPRINTLN1 (config.PushoverDevices);
  DEBUGPRINT1 ("token: "); DEBUGPRINT1 (config.PushoverToken); DEBUGPRINT1 ("  Key: "); DEBUGPRINTLN1 (config.PushoverUserkey);

  Pushover po = Pushover (config.PushoverToken,config.PushoverUserkey);
  po.setDevice (config.PushoverDevices);
  po.setMessage (messageText);
  po.setSound ("bike");
  po.setTitle ("Home Alone");
  po.setPriority (prio);
  ret = po.send();

  DEBUGPRINT1 ("returcode Pushover: "); 
  DEBUGPRINTLN1 (ret); //should return 1 on success
  
  return (ret);
}


// end of cde -----------------------------------



// end of code ------------------------------
