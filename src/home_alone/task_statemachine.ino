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

int ath_count;
int ath_count_day;
bool do_report= false;


int mvcount;

time_t now_4;
static time_t last_time_away_report;

time_t leaving_start;
time_t now_3;
#define WAIT_TO_REPORT 10000



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

    }

// stuff we do irrelevant of state --------
//-----------------------------------------
    time(&now);
    localtime_r (&now, &timeinfo);
  
    curr_hour = timeinfo.tm_hour;
    curr_dayofyear = timeinfo.tm_yday;
    curr_year = timeinfo.tm_year -100;   // function returns year since 1900


    xSemaphoreTake(SemaMovement, portMAX_DELAY);
        timelastmv = timelastMovement;
     xSemaphoreGive(SemaMovement);
         

    if ((now - timelastmv) >= (config.HoursbetweenNoMovementRep * 3600)) {
         DEBUGPRINTLN1 ("no movement for the last period");
          // assemble string to be displayed   
        sprintf( buf , "Watch out: no movement in last period");
        
      // push morning message ------------
          push_msg (buf,1);             // High priority message       

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
  
    case NIGHT:
      do_night();
      break;

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
       ath_count = movement_count;
       ath_count_day = movement_count_perday;
       timelastmv = timelastMovement;                 // only needed for test runs
       xSemaphoreGive(SemaMovement);

   }
    else  {                                             // semaphore busy, do nothing....
       DEBUGPRINTLN1 ("Semaphore clock 1busy");
    }

    if (do_report) {
      DEBUGPRINT1 ("trying to report to cloud, count:  ");   DEBUGPRINT1 (ath_count); DEBUGPRINT1 (" / "); DEBUGPRINTLN1 (ath_count_day); 
       do_report = false;

         // set up parameter for this job
            wifi_todo = REPORT_CLOUD;
            wifi_order_struct.order = wifi_todo;
            wifi_order_struct.mvcount  = ath_count;
            ret = wifi_func();
            DEBUGPRINT2 ("wifi_func returns: ");   DEBUGPRINTLN2 (ret);
 
  vTaskDelay(200 / portTICK_PERIOD_MS);

          if (ret == 0) {           // reset count if ok 
            xSemaphoreTake(SemaMovement, portMAX_DELAY);
            movement_count = 0;
            xSemaphoreGive(SemaMovement);
          }

//  if we are running debu, we print time last movement

      if (debug_flag) {
       DEBUGPRINT1 ("last movement at: "); DEBUGPRINTLN1 (timelastmv);
      }
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
      test_push ("Message: Testbutton", 1);           // use priority 1 HIGH
    }



// check if quitehours reached -------------------------
// change state to night if this happened---------------
  
   
    if (curr_hour >= config.QuietHoursStart) {
      state= NIGHT;
      athome_first_time = true;   
    }
    
    
    vTaskDelay(500 / portTICK_PERIOD_MS);
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

    if (curr_hour >= config.QuietHoursStart) {
      state= NIGHT;
      away_first_time = true;   
    }

    
   vTaskDelay(200 / portTICK_PERIOD_MS);
   

  // check if movement 
   xSemaphoreTake(SemaMovement, portMAX_DELAY);
   mvcount= movement_count;
   xSemaphoreGive(SemaMovement);
   if (mvcount > 0) {
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
      movement_count = 0;
      xSemaphoreGive(SemaMovement);
      state = AWAY;
      leaving_first_time = true; 
      DEBUGPRINTLN1 ("end state leaving, next state: away");    
    }
 
     vTaskDelay(1000 / portTICK_PERIOD_MS);

}

//---------------------------------------------------------------

//---------------------------------------------------------------
//----------------------------------------------------------------
// this function handles the state 'night' from the state machine
//---------------------------------------------------------------
//---------------------------------------------------------------
void do_night() {

  int next_state = ATHOME;
  
     if (night_first_time) {
        DEBUGPRINT1 ("STATE night - Running on core:");
        DEBUGPRINTLN1 (xPortGetCoreID());
        night_first_time = false;
        value4_oled = 4;              // signal is night
        xSemaphoreTake(SemaOledSignal, portMAX_DELAY);    // signal oled task to switch display on
        oledsignal = 1;
        xSemaphoreGive(SemaOledSignal);
        time(&last_time_away_report);

        day_night_old_dayofyear = curr_dayofyear;       // keep beginning of night
        day_night_old_year = curr_year;   // function returns year since 1900
        
        DEBUGPRINTLN1 ("Good night...");
        
        if (debug_flag_push)     test_push  ("Message: begin Quiethours", -1);     // do this if modus is test
    }


        int but = digitalRead(button_awayPin); // read input value
    if (but == LOW) { // check if the input is HIGH
      DEBUGPRINTLN1 ("button away pressed");
      next_state= AWAY;
      night_first_time = true;  
      }    
 
     DEBUGPRINTLN3 ("night...");


//  we do nothing during quiet hours
//  after morning we change to state away where we remain until the first movement
  

    vTaskDelay(3000 / portTICK_PERIOD_MS);
    


//-------------------------------------------------
// check if morning has broken, quiet hour ended
//------------------------------------------------

    
  DEBUGPRINT3 (curr_dayofyear);  DEBUGPRINT3 (" / "); DEBUGPRINTLN3 (day_night_old_dayofyear); 
  DEBUGPRINT3 (curr_year);  DEBUGPRINT3 (" / ");  DEBUGPRINTLN3 (day_night_old_year); 
  DEBUGPRINT3 (curr_hour);  DEBUGPRINT3 (" / ");  DEBUGPRINTLN3 (config.QuietHoursEnd); 

   if ((  day_night_old_year == curr_year) and (day_night_old_dayofyear == curr_dayofyear)) {
        DEBUGPRINTLN3 ("No day and no year change");                        // value 2 für debug
   }
   else if (curr_hour >= config.QuietHoursEnd) {
      state= next_state;                      // change state
      night_first_time = true;   
      DEBUGPRINTLN1 ("Good morning...");
      if (debug_flag_push)     test_push("Message: Morning has broken", -1);     // do this if modus is test
    }
 

}
//---------------------------------------
//




// end of code
//---------------------------------------------------
