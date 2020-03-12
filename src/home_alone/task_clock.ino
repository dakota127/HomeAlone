/* -------------------------------------------------------------------
* Home Alone Application
* Based on a project presentd by Ralph Bacon
* This version used an ESP32 doing multitasking.
* by Peter B, from Switzerland
* Project website http://projects.descan.com/projekt7.html

*/



static bool done_morningreporting = false;           // have we already done it at the specified hour ?
static bool done_eveningreporting = false;

static bool done_clock_tick_1 = false;           // have we already done it at the specified hour ?
static bool done_clock_tick_2 = false;
static bool done_clock_tick_3 = false;

static time_t last_time_clock_1;

char buf[20];
//
//--------------------------------------------------------- 
// function task_clock(), runs as a sepatare task ------------
//----------------------------------------------------------
void task_clock ( void * parameter )
{
 
  int count;
  int count_day;
      
 static bool clock_firsttime = true ;
  for (;;) {

 // do first time through.......   
    if (clock_firsttime) {
      DEBUGPRINT2 ("\t\tTASK clock - Running on core: ");
      DEBUGPRINTLN2 (xPortGetCoreID());
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
       
    DEBUGPRINT3 ("\t\t");  DEBUGPRINT3 ("old_hour:");  DEBUGPRINT3 (old_hour);   DEBUGPRINT3 (" old_dayofyear: ");   DEBUGPRINT3 (old_dayofyear);  DEBUGPRINT3 (" old_year: ");   DEBUGPRINTLN3 (old_year); 
    DEBUGPRINT3 ("\t\t");  DEBUGPRINT3 ("curr_hour:");  DEBUGPRINT3 (curr_hour);   DEBUGPRINT3 (" curr_dayofyear: ");   DEBUGPRINT3 (curr_dayofyear); DEBUGPRINT3 (" curr_year: ");   DEBUGPRINTLN3 (curr_year);  

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

   if (config.MorningReportingHour > 0) {
    
    
   if ((  old_year <= curr_year) or (old_dayofyear != curr_dayofyear)) {
      if  ((curr_hour >= config.MorningReportingHour) and (done_morningreporting == false)) {

           // assemble string to be displayed   
        sprintf( buf , " Good Morning, Mov: %d /  %d", count, count_day);
        
      // push morning message ------------
          push_msg (buf, -1);
          done_morningreporting = true;
          old_dayofyear  = timeinfo.tm_yday;
          old_year = timeinfo.tm_year -100;   
    }
   }
   else {
        DEBUGPRINT3 ("\t\t");
        DEBUGPRINTLN3 ("No day and no year change 2");                        // value 2 für debug
   }

   }
    
// end handling Morning reporting

//---------------------------------------------------------------------------------------
// -----  handle clock_3 (used for evening  reporting (pushing a message)-----------------
//---------------------------------------------------------------------------------------

  if (config.EveningReportingHour > 0) {
    
 
   if ((  old_year <= curr_year) or (old_dayofyear != curr_dayofyear)) {
      if  ((curr_hour >= config.EveningReportingHour) and (done_eveningreporting == false)) {

        // assemble string to be displayed   
         sprintf( buf , " Good Evening, Mov: %d /  %d", count, count_day);
        
      // push Evening message ------------
          push_msg (buf , -1);
          done_eveningreporting = true;
          old_dayofyear  = timeinfo.tm_yday;
          old_year = timeinfo.tm_year -100;   
    
    }
   }
   else {
        DEBUGPRINTLN3 ("\t\tNo day and no year change 3");                        // value 2 für debug
   }

  }  
// end handling Morning reporting ----------------------------




   vTaskDelay(600 / portTICK_PERIOD_MS);
  }
}  // end function





//--------------------------------------------------------------
int push_msg (String text, int prio){
  

// ------- report to pushover ----------------------------------

     
    DEBUGPRINTLN1 ("\t\tclock send Pushover message");


 // we need to report to the cloud

         // set up parameter for this job
        wifi_todo = PUSH_MESG;
        wifi_order_struct.order = wifi_todo;
        wifi_order_struct.pushtext = text;
        wifi_order_struct.priority = prio;    
        ret = wifi_func();
        DEBUGPRINT2 ("\t\twifi_func returns: ");   DEBUGPRINTLN2 (ret);


        value3_oled = 6;   
        xSemaphoreTake(SemaOledSignal, portMAX_DELAY);    // signal oled task to switch display on
         oledsignal = 1;
        xSemaphoreGive(SemaOledSignal);        
        vTaskDelay(200 / portTICK_PERIOD_MS);
   
      
      vTaskDelay(100 / portTICK_PERIOD_MS);

     if (ret == 0) {           // reset count if ok 
        DEBUGPRINTLN1 ("\t\tmessage sent ok");
     }
// ------- report to pushover ----------------------------------
     
}

// end of code
//---------------------------------------------------
