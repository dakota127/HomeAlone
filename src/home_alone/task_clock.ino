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



// constants won't change:


static bool done_morningreporting = false;           // have we already done it at the specified hour ?
static bool done_eveningreporting = false;

static bool done_clock_tick_1 = false;           // have we already done it at the specified hour ?
static bool done_clock_tick_2 = false;
static bool done_clock_tick_3 = false;

static time_t last_time_clock_1;

char buf[20];
//
//--------------------------------------------------------- 
// function metronome, runs as a sepatare task ------------
//----------------------------------------------------------
void task_clock ( void * parameter )
{
 
  int count;
  int count_day;
      
 static bool clock_firsttime = true ;
  for (;;) {

 // do first time through.......   
    if (clock_firsttime) {
      DEBUGPRINT1 ("\t\tTASK clock - Running on core: ");
      DEBUGPRINTLN1 (xPortGetCoreID());
      clock_firsttime = false;
      time(&last_time_clock_1);            
      vTaskDelay(5000 / portTICK_PERIOD_MS);
       // nothing else to do ....
    }


 // get all the current time things....
    time(&now);
    localtime_r (&now, &timeinfo);
    curr_hour = timeinfo.tm_hour;
    curr_dayofyear = timeinfo.tm_yday;
    curr_year = timeinfo.tm_year -100;   // function returns year since 
       
    DEBUGPRINT2 ("\t\t");  DEBUGPRINT2 ("old_hour:");  DEBUGPRINT2 (old_hour);   DEBUGPRINT2 (" old_dayofyear: ");   DEBUGPRINT2 (old_dayofyear);  DEBUGPRINT2 (" old_year: ");   DEBUGPRINTLN2 (old_year); 
    DEBUGPRINT2 ("\t\t");  DEBUGPRINT2 ("curr_hour:");  DEBUGPRINT2 (curr_hour);   DEBUGPRINT2 (" curr_dayofyear: ");   DEBUGPRINT2 (curr_dayofyear); DEBUGPRINT2 (" curr_year: ");   DEBUGPRINTLN2 (curr_year);  

// get the movement counts (protected by semaphore

        xSemaphoreTake(SemaMovement, portMAX_DELAY);
        count = movement_count;
        count_day = movement_count_perday;
        xSemaphoreGive(SemaMovement);

//---------------------------------------------------------------------------------------
// -----  handle clock_1 (used for upload data to cloud))------------
//---------------------------------------------------------------------------------------

    if (( now - last_time_clock_1) > config.MinutesBetweenUploads ) {   
            
        time(&last_time_clock_1);              // store time 
  // get semaphore and set the clock -------------------
      if( xSemaphoreTake( clock_1Semaphore, ( TickType_t ) 10 ) == pdTRUE )  {     // semaphore obtained, now do the work
       DEBUGPRINTLN1 ("\t\ttask clock setze clock_1");
       clock_tick_1 = true;
       vTaskDelay(10 / portTICK_PERIOD_MS);
       xSemaphoreGive( clock_1Semaphore );              // release semaphore
        }
        else  {                                             // semaphore busy, do nothing....
        Serial.println("\t\tclock1 - Semaphore busy");
        }

    }     

        

//---------------------------------------------------------------------------------------
// -----  handle clock_2 (used for morning reporting (pushing a message))-----------------
//---------------------------------------------------------------------------------------
   if ((  old_year <= curr_year) or (old_dayofyear != curr_dayofyear)) {
      if  ((curr_hour >= config.MorningReportingHour) and (done_morningreporting == false)) {

           // assemble string to be displayed   
        sprintf( buf , " Good Morning, Mov: %d /  %d", count, count_day);
        
      // push morning message ------------
          push_msg (buf);
          done_morningreporting = true;
          old_dayofyear  = timeinfo.tm_yday;
          old_year = timeinfo.tm_year -100;   
    }
   }
   else {
        DEBUGPRINT2 ("\t\t");
        DEBUGPRINTLN2 ("No day and no year change 2");                        // value 2 für debug
   }
// end handling Morning reporting

//---------------------------------------------------------------------------------------
// -----  handle clock_3 (used for evening  reporting (pushing a message)-----------------
//---------------------------------------------------------------------------------------
   if ((  old_year <= curr_year) or (old_dayofyear != curr_dayofyear)) {
      if  ((curr_hour >= config.EveningReportingHour) and (done_eveningreporting == false)) {

        // assemble string to be displayed   
         sprintf( buf , " Good Evening, Mov: %d /  %d", count, count_day);
        
      // push Evening message ------------
          push_msg (buf);
          done_eveningreporting = true;
          old_dayofyear  = timeinfo.tm_yday;
          old_year = timeinfo.tm_year -100;   
    
    }
   }
   else {
        DEBUGPRINTLN2 ("\t\tNo day and no year change 2");                        // value 2 für debug
   }
// end handling Morning reporting ----------------------------




   vTaskDelay(600 / portTICK_PERIOD_MS);
  }
}  // end function





//--------------------------------------------------------------
int push_msg (String text){
  

// ------- report to pushover ----------------------------------

     
    DEBUGPRINTLN1 ("\t\tclock send Pushover message");


 // we need to report to the cloud - this is done in the wifi task
 // first we need to se if task is free or busy - we check the tasks semaphore

     /* See if we can obtain the semaphore.  If the semaphore is not
        available wait 10 ticks to see if it becomes free. */
     if( xSemaphoreTake( wifi_semaphore, ( TickType_t ) 100 ) == pdTRUE ) {
            /* We were able to obtain the semaphore and can now access the
            shared resource. */
         // set up parameter for this job
        wifi_todo = PUSH_MESG;
        wifi_order_struct.order = wifi_todo;
        wifi_order_struct.pushtext = text;
            
       //      strlcpy(wifi_order_struct.pushtext, text,   sizeof(text));                    // <- destination
     
  //      Serial.println (text);
  //      Serial.println (wifi_order_struct.pushtext);
        vTaskResume( Task1 );
            /* We have finished accessing the shared resource.  Release the
            semaphore. */
    //        xSemaphoreGive( wifi_semaphore );
      }
      else  {
            /* We could not obtain the semaphore and can therefore not access
            the shared resource safely. */

        DEBUGPRINTLN2 ("\t\twifi semaphore busy (push msg)");
        vTaskDelay(200 / portTICK_PERIOD_MS);
      }
 
      
      vTaskDelay(100 / portTICK_PERIOD_MS);
     //     int res = report_toPushover("Im Westen nichts Neues     ");  
     int res = 0;
     if (res == 0) {           // reset count if ok 
        DEBUGPRINTLN1 ("\t\tmessage sent ok");
     }
// ------- report to pushover ----------------------------------
     
}

// end of code
//---------------------------------------------------
