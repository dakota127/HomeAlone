/* -------------------------------------------------------------------
* Home Alone Application
* Based on a project presentd by Ralph Bacon
* This version used an ESP32 doing multitasking.
* by Peter B, from Switzerland
* Project website http://projects.descan.com/projekt7.html
* 
* THIS Function RUNS as a separat task
* It implements the state machine
*
* ------------------------------------------------------------------- 
*/



static bool night_first_time = true;                // first_time run through a state, bitwise, bit 0 means state 1
static bool away_first_time = true;                // first_time run through a state, bitwise, bit 0 means state 1
static bool athome_first_time = true;                // first_time run through a state, bitwise, bit 0 means state 1
static bool leaving_first_time = true;                // first_time run through a state, bitwise, bit 0 means state 1

int mvcount_push;
int mvcount_cloud;
bool do_report= false;


time_t now_4;
static time_t last_time_away_report;

time_t leaving_start;
time_t now_3;
#define WAIT_TO_REPORT 10000

char bufpush[80];

int retcode;
int timelastmv;

//-----------------------------------------------------------
//-----------------------------------------------------------
// implementation of the state machine (Runs in separate task !)
//-----------------------------------------------------------
//-----------------------------------------------------------
void state_machine( void * parameter )
{
 static bool main_firsttime = true ;
 
  for (;;) {

    if (main_firsttime) {
      DEBUGPRINT2 ("TASK state_machine - Running on core:");
      DEBUGPRINTLN2 (xPortGetCoreID());
      main_firsttime = false;
      state = ATHOME;                   // initial state of state machine
      // set this to prevent reporting if boot during the day. We want reporting only the next day
      if  (curr_hour >= config.MorningReportingHour)   done_morningreporting = true;
      if  (curr_hour >= config.EveningReportingHour)   done_eveningreporting = true;
        DEBUGPRINT0 ("M-rep: ");  DEBUGPRINT0 (done_morningreporting);  DEBUGPRINT0 ("  E-rep: ");  DEBUGPRINTLN0 (done_eveningreporting); 
      
    }

// stuff we do irrelevant of state --------
//-----------------------------------------

// is_newday is set as soon as a new day has begun
// it will stay on for one single interation of the loop
    is_newday = false;

// get current date info --------
    time(&now);
    localtime_r (&now, &timeinfo);

    curr_hour = timeinfo.tm_hour;
    curr_dayofyear = timeinfo.tm_yday;
    curr_year = timeinfo.tm_year -100;   // function returns year since 1900

// check if new day has arrived 
    if ((old_year == curr_year) and (old_dayofyear == curr_dayofyear)) {
        DEBUGPRINTLN3 ("No day and no year change");                        // value 2 für debug
    }
    else {
      DEBUGPRINTLN2 ("a new day has begun");   
      old_year = curr_year;
      old_dayofyear = curr_dayofyear;
      is_newday = true;
      done_morningreporting = false;      // reset this for the new day that has just started
      done_eveningreporting = false;      // reset this for the new day
    }
  

    xSemaphoreTake(SemaMovement, portMAX_DELAY);
      timelastmv = timelastMovement;
    xSemaphoreGive(SemaMovement);
         

    if ((now - timelastmv) >= (config.HoursbetweenNoMovementRep * 3600)) {
         DEBUGPRINTLN1 ("no movement for the last period");
          // assemble string to be displayed   
        sprintf (bufpush, "Keine Bewegung in den letzten %d Stunden", config.HoursbetweenNoMovementRep);
        
      // push alert message ------------
         retcode = push_msg (bufpush,1);             // High priority message  in red     
         DEBUGPRINT1 ("push_msg() returns: "); DEBUGPRINTLN1 (retcode);
        xSemaphoreTake(SemaMovement, portMAX_DELAY);
          timelastMovement = now;
        xSemaphoreGive(SemaMovement);
      
    }




// state machine implementation starts here
// switch according to state variable

  switch (state) {

    case ATHOME:
      do_athome();
      break;

    case LEAVING:
      do_leaving();
      break;

    case AWAY:
      do_away();
      break;
 /* 
    case NIGHT:
      do_night();
      break;
*/
    default:
      DEBUGPRINTLN0 ("case main zu default, error!");
      vTaskDelay(200 / portTICK_PERIOD_MS);
      break;
   }              // end case stmt

    vTaskDelay(200 / portTICK_PERIOD_MS);

  }
}


//
//

//---------------------------------------------------------------
//----------------------------------------------------------------
// this function handles the state 'athome' from the state machine
//---------------------------------------------------------------
//---------------------------------------------------------------
void do_athome() {

 

    if (athome_first_time) {
      DEBUGPRINT2("STATE athome - Running on core:");
      DEBUGPRINTLN2(xPortGetCoreID());
      athome_first_time = false;
      value4_oled = 1;              // signal is at home
      xSemaphoreTake(SemaOledSignal, portMAX_DELAY);    // signal oled task to switch display on
      oledsignal = 1;
      xSemaphoreGive(SemaOledSignal);
      
   
      DEBUGPRINT1 ("Upload intervall (sec): ");
      DEBUGPRINTLN1 (config.MinutesBetweenUploads);
      DEBUGPRINT1 ("Msg after no movement (hours): ");
      DEBUGPRINTLN1 (config.HoursbetweenNoMovementRep);
       // nothing else to do ....
    }


 

  // get semaphore and read the clock number 1 ------------------
   if( xSemaphoreTake( clock_1Semaphore, ( TickType_t ) 10 ) == pdTRUE )  {     // semaphore obtained, now do the work
       if (clock_tick_1) {
           DEBUGPRINTLN1  ("Clock 1 ticked....");
           clock_tick_1 = false;
           do_report = true;
           }
       vTaskDelay(10 / portTICK_PERIOD_MS);
       xSemaphoreGive( clock_1Semaphore );              // release semaphore

// get movement count ----------
       xSemaphoreTake(SemaMovement, portMAX_DELAY);
       mvcount_cloud = movCount_reportingPeriod_cloud;
       timelastmv = timelastMovement;                 // only needed for test runs
       xSemaphoreGive(SemaMovement);

   }
    else  {                                             // semaphore busy, do nothing....
       DEBUGPRINTLN1 ("Semaphore clock 1busy");
    }

    if (do_report) {
      DEBUGPRINT1 ("trying to report to cloud, count:  ");   DEBUGPRINTLN1 (mvcount_cloud); 
       do_report = false;

         // set up parameter for this job
            wifi_todo = REPORT_CLOUD;
            wifi_order_struct.order = wifi_todo;
            wifi_order_struct.mvcount  = mvcount_cloud;
            retcode = wifi_func();
            DEBUGPRINT2 ("wifi_func returns: ");   DEBUGPRINTLN2 (retcode);
 
            vTaskDelay(200 / portTICK_PERIOD_MS);

          if (retcode == 0) {           // reset count if ok 
            xSemaphoreTake(SemaMovement, portMAX_DELAY);
            movCount_reportingPeriod_cloud = 0;
            xSemaphoreGive(SemaMovement);
          }

//  if we are running debu, we print time last movement

      if (debug_flag) {
       DEBUGPRINT1 ("last movement at: "); DEBUGPRINTLN1 (timelastmv);
      }

     vTaskDelay(4000 / portTICK_PERIOD_MS);        // eingefügt wegen konflikt wifi
// ------- report to cloud ----------------------------------
    }
         



    int but = digitalRead(button_awayPin); // read input value
    if (but == LOW) { // check if the input is HIGH
      DEBUGPRINTLN1 ("button away pressed");
      state= LEAVING;
      athome_first_time = true;  
      }

//   char time_output[30];
//    strftime(time_output, 30, "%a  %d-%m-%y %T", localtime(&now2));

// check if test button is pressed, send pushmesage if so....--------
    but = digitalRead(button_test); // read input value
    if (but == LOW) { // check if the input is HIGH
      DEBUGPRINTLN1 ("button test pressed");

      // get movement count
       xSemaphoreTake(SemaMovement, portMAX_DELAY);
       mvcount_push = movCount_reportingPeriod_push;
       xSemaphoreGive(SemaMovement);
      
      sprintf( bufpush , "Testmeldung, Last Reset: %s (%d) count: %d   ", time_lastreset, lastResetreason, mvcount_push);
      retcode = push_msg (bufpush, 1);           // use priority 1 HIGH
      DEBUGPRINT1 ("push_msg() returns: "); DEBUGPRINTLN1 (retcode);
    }


   if (config.MorningReportingHour > 0) {
      
    if  ((curr_hour >= config.MorningReportingHour) and (done_morningreporting == false)) {
        DEBUGPRINTLN2 ("Morning Reporting");     

// get movement count ----------
       xSemaphoreTake(SemaMovement, portMAX_DELAY);
       mvcount_push= movCount_reportingPeriod_push;
       xSemaphoreGive(SemaMovement);  
              
           // assemble string to be displayed   
       sprintf( bufpush , "Guten Morgen, Anzahl Bewegungen: %d   ", mvcount_push);
        
      // push morning message ------------
       retcode = push_msg (bufpush, 0);       // prio zero
       DEBUGPRINT1 ("push_msg() returns: "); DEBUGPRINTLN1 (retcode);
       if (retcode == 0)  {
            xSemaphoreTake(SemaMovement, portMAX_DELAY);
            movCount_reportingPeriod_push = 0;
            xSemaphoreGive(SemaMovement); 
            done_morningreporting = true;
        }
        else  vTaskDelay(1000 / portTICK_PERIOD_MS);        
     
    }
  
   }
     
// end handling Ev rening eporting

   if (config.EveningReportingHour > 0) {
      
    if  ((curr_hour >= config.EveningReportingHour) and (done_eveningreporting == false)) {

        DEBUGPRINTLN2 ("Evening Reporting");     

// get movement count ----------
       xSemaphoreTake(SemaMovement, portMAX_DELAY);
       mvcount_push= movCount_reportingPeriod_push;
       xSemaphoreGive(SemaMovement);

        // assemble string to be displayed   
       sprintf( bufpush , "Gute Nacht, Anzahl Bewegungen: %d   ", mvcount_push);
        
      // push evening message ------------
       retcode = push_msg (bufpush, 0);       // prio zero
       DEBUGPRINT1 ("push_msg() returns: "); DEBUGPRINTLN1 (retcode);
       if (retcode == 0)  {
            xSemaphoreTake(SemaMovement, portMAX_DELAY);
            movCount_reportingPeriod_push = 0;
            xSemaphoreGive(SemaMovement); 
            
            done_eveningreporting = true;
        }
        else  vTaskDelay(1000 / portTICK_PERIOD_MS);
       
    }
 
   }   
// end handling Morning reporting

    
    vTaskDelay(100 / portTICK_PERIOD_MS);
//  DEBUGPRINTLN1 ("do_athome(

}


//---------------------------------------------------------------
//----------------------------------------------------------------
// this function handles the state 'away' from the state machine
//---------------------------------------------------------------
//---------------------------------------------------------------
void do_away() {

     if (away_first_time) {
        DEBUGPRINT2 ("STATE away - Running on core:");
        DEBUGPRINTLN2 (xPortGetCoreID());
        away_first_time = false;
        value4_oled = 3;              // signal is away
        xSemaphoreTake(SemaOledSignal, portMAX_DELAY);    // signal oled task to switch display on
        oledsignal = 1;
        xSemaphoreGive(SemaOledSignal);
        time(&last_time_away_report);
    }
     DEBUGPRINTLN3 ("away1...");


// ------- report to cloud ----------------------------------
    time(&now_4);
    if (( now_4 - last_time_away_report) > config.MinutesBetweenUploads ) {         
          time(&last_time_away_report);
          DEBUGPRINTLN2 ("away report this fact");

          int res = report_toCloud(-2);  
     
        }
// ------- report to cloud ----------------------------------

/*
    if (curr_hour >= config.QuietHoursStart) {
      state= NIGHT;
      away_first_time = true;   
    }
*/
    
   vTaskDelay(200 / portTICK_PERIOD_MS);
   

  // check if movement 
   xSemaphoreTake(SemaMovement, portMAX_DELAY);
   mvcount_cloud= movCount_reportingPeriod_cloud;
   xSemaphoreGive(SemaMovement);
   if (mvcount_cloud > 0) {
     state = ATHOME;
     away_first_time = true; 
     DEBUGPRINTLN1 ("movement count > 0 leaving state away ");
    }

  vTaskDelay(300 / portTICK_PERIOD_MS);
    

}
//---------------------------------------------------------------

//---------------------------------------------------------------
//----------------------------------------------------------------
// this function handles the state 'leaving' from the state machine
//---------------------------------------------------------------
//---------------------------------------------------------------
void do_leaving() {
    if (leaving_first_time) {
      DEBUGPRINT2 ("STATE leaving - Running on core:");
      DEBUGPRINTLN2 (xPortGetCoreID());
      DEBUGPRINT2 ("timeout (sec): ");
      DEBUGPRINTLN2 (config.TimeOutLeavingSec);
      leaving_first_time = false;
       value4_oled = 2;              // signal is at leaving
      xSemaphoreTake(SemaOledSignal, portMAX_DELAY);    // signal oled task to switch display on
      oledsignal = 1;
      xSemaphoreGive(SemaOledSignal);
      
      time(&leaving_start);
      xSemaphoreTake(SemaButton, portMAX_DELAY);
       button_awaypressed = 0;
      xSemaphoreGive(SemaButton);
      vTaskDelay(WAIT_TO_REPORT / portTICK_PERIOD_MS);          // wait some time until reporting leaving

// ----- report to cloud -------------------------------     
     int res = report_toCloud(-5);
//------------------------------------------------------
    
    }


    time(&now_3);
  
    if (( now_3 - leaving_start) > (config.TimeOutLeavingSec) ) {     // ok, we leave this state
     Serial.println ( now_3 - leaving_start);
      xSemaphoreTake(SemaMovement, portMAX_DELAY);
      movCount_reportingPeriod_cloud = 0;
      xSemaphoreGive(SemaMovement);
      state = AWAY;
      leaving_first_time = true; 
      DEBUGPRINTLN1 ("end state leaving, next state: away");    
    }
 
     vTaskDelay(1000 / portTICK_PERIOD_MS);

}

//---------------------------------------------------------------

//---------------------------------------------------------------




// end of code
//---------------------------------------------------
