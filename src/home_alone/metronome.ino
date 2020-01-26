/*
* Home Alone Application
* Based on a project presentd by Ralph Bacon

* This version based on a state machine
* 
* THIS RUNS as a separat task
* does nothing for now 
* 
* 
*/

TickType_t g_time_last_check;

// constants won't change:


static bool done_morningreporting = false;           // have we already done it at the specified hour ?
static bool done_eveningreporting = false;
//
//--------------------------------------------------------- 
// function metronome, runs as a sepatare task ------------
//----------------------------------------------------------
void metronome( void * parameter )
{
 
 
 static bool alarm_firsttime = true ;
  for (;;) {
    
    if (alarm_firsttime) {

      DEBUGPRINT1 ("TASK metronome - Running on core:");
      DEBUGPRINTLN1 (xPortGetCoreID());
      g_time_last_check = xTaskGetTickCount();                // not used
      alarm_firsttime = false;
      vTaskDelay(5000 / portTICK_PERIOD_MS);
       // nothing else to do ....
    }
 
 

// -----  handle morning and evening reporting (pushing a message)-----------------
   int currenthour = timeinfo.tm_hour;

   if ((currenthour >= config.EveningReportingHour) and (done_eveningreporting == false)) {
      done_eveningreporting = true;
      done_morningreporting = false;

      DEBUGPRINTLN1 ("trying evening reporting");
    
  // do it
    int ret = push_msg ("Guten Abend.....");
        
      DEBUGPRINTLN1 ("done evening reporting");
    }

  if ((currenthour >= config.MorningReportingHour) and (done_morningreporting == false)) {
      done_morningreporting = true;
      done_eveningreporting = false;

      DEBUGPRINTLN1 ("trying morning reporting");
  // do it
     int ret = push_msg ("Guten Morgen....");  
      
      DEBUGPRINTLN1 ("done morning reporting");
    }



   vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}  // end function



//--------------------------------------------------------------
int push_msg (String text){
  

// ------- report to pushover ----------------------------------
    time(&now_2);
    if (( now_2 - last_time_push) > config.MinutesBetweenPushover ) {         
    
    time(&last_time_push);
              
    DEBUGPRINTLN1 ("metronome send Pushover message");


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
            wifi_order_struct.pushtext = text;
            
       //      strlcpy(wifi_order_struct.pushtext, text,   sizeof(text));                    // <- destination
     
            Serial.println (text);
            Serial.println (wifi_order_struct.pushtext);
            vTaskResume( Task1 );
            /* We have finished accessing the shared resource.  Release the
            semaphore. */
    //        xSemaphoreGive( wifi_semaphore );
        }
        else
        {
            /* We could not obtain the semaphore and can therefore not access
            the shared resource safely. */

            Serial.println ("wifi semaphore busy (push msg)");
            vTaskDelay(200 / portTICK_PERIOD_MS);

        }
 
       
      
      
      vTaskDelay(100 / portTICK_PERIOD_MS);
     //     int res = report_toPushover("Im Westen nichts Neues     ");  
     int res = 0;
          if (res == 0) {           // reset count if ok 
            DEBUGPRINTLN1 ("message sent ok");
          }
// ------- report to mail ----------------------------------
     
    }  // end Zeit erreicht

}

// end of code
//---------------------------------------------------
