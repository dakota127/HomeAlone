/*
* Home Alone Application
* Based on a project presentd by Ralph Bacon

* This version based on a state machine
* 
*  THIS RUNS in TASK state_machine !
* LEAVING is one of the states of the machine
* we are in this state if somebody pushed the I am leaving button
* 
* upon entering this state we report this the cloud with value -10
* We stay in this state for n minutes, no other condition can make us leave this state
* we do not check on motion
* 
* 
*/


static bool leaving_first_time = true;                // first_time run through a state, bitwise, bit 0 means state 1

time_t leaving_start;
time_t now_3;
#define WAIT_TO_REPORT 10000

//---------------------------------------------------------------
// this function handles the state 'leaving' from the state machine
//---------------------------------------------------------------
void do_leaving() {
    if (leaving_first_time) {
      DEBUGPRINT1 ("STATE leaving - Running on core:");
      DEBUGPRINTLN1 (xPortGetCoreID());
      DEBUGPRINT1 ("timeout (sec): ");
      DEBUGPRINTLN1 (config.TimeOutPeriodSec);
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
  
    if (( now_3 - leaving_start) > (config.TimeOutPeriodSec) ) {     // ok, we leave this state
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

// -- end of code ---------------------
