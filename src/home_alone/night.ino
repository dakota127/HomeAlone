/*
* Home Alone Application
* Based on a project presentd by Ralph Bacon

* This version based on a state machine
* 
* THIS Function RUNS in TASK state_machine !
* AWAY is one of the states of the machine
* we are in this state if nobody is home
* 
* We report this fact to the cloud every n minutes with field1 = -5
* upon detecting a motion the machine switches to the ATHOME state (no burglars are reported)
* if no movement the machine remains in this state
* 
*/


static bool night_first_time = true;                // first_time run through a state, bitwise, bit 0 means state 1

//
//---------------------------------------------------------------
// this function handles the state 'night' from the state machine
//---------------------------------------------------------------
void do_night() {

     if (night_first_time) {
        DEBUGPRINT1 ("STATE night - Running on core:");
        DEBUGPRINTLN1 (xPortGetCoreID());
        night_first_time = false;
        value4_oled = 4;              // signal is night
        xSemaphoreTake(SemaOledSignal, portMAX_DELAY);    // signal oled task to switch display on
        oledsignal = 1;
        xSemaphoreGive(SemaOledSignal);
        time(&last_time_away_report);
    }
     DEBUGPRINTLN1 ("night...");


//  we do nothing during quiet hours
//  after morning we change to state away where we remain until the first movement

    vTaskDelay(200 / portTICK_PERIOD_MS);

  

  vTaskDelay(3000 / portTICK_PERIOD_MS);
    
// check if morning, quiet hour ended

  time(&now);
  localtime_r (&now, &timeinfo);
  int curr_hour = timeinfo.tm_hour;
  int curr_dayofyear = timeinfo.tm_yday;
  int curr_year = timeinfo.tm_year -100;   // function returns year since 
    
  DEBUGPRINT2 (curr_dayofyear);  DEBUGPRINT2 (" / "); DEBUGPRINTLN2 (old_dayofyear); 
  DEBUGPRINT2 (curr_year);  DEBUGPRINT2 (" / ");  DEBUGPRINTLN2 (old_year); 
  DEBUGPRINT2 (curr_hour);  DEBUGPRINT2 (" / ");  DEBUGPRINTLN2 (config.QuietHoursEnd); 

   if ((  old_year == curr_year) and (old_dayofyear == curr_dayofyear)) {
        DEBUGPRINTLN2 ("No day and no year change");                        // value 2 für debug
   }
   else if (curr_hour > config.QuietHoursEnd) {
      state= AWAY;                      // we change to state away and wait there for first movement
      night_first_time = true;   
      DEBUGPRINTLN1 ("Good morning...");
    }
 

}
//---------------------------------------
//
