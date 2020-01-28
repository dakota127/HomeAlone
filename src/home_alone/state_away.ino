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


static bool away_first_time = true;                // first_time run through a state, bitwise, bit 0 means state 1
int mvcount;

time_t now_4;
static time_t last_time_away_report;
//
//
//---------------------------------------------------------------
// this function handles the state 'away' from the state machine
//---------------------------------------------------------------
void do_away() {

     if (away_first_time) {
        DEBUGPRINT1 ("STATE away - Running on core:");
        DEBUGPRINTLN1 (xPortGetCoreID());
        away_first_time = false;
        value4_oled = 3;              // signal is away
        xSemaphoreTake(SemaOledSignal, portMAX_DELAY);    // signal oled task to switch display on
        oledsignal = 1;
        xSemaphoreGive(SemaOledSignal);
        time(&last_time_away_report);
    }
     DEBUGPRINTLN1 ("away1...");


// ------- report to cloud ----------------------------------
    time(&now_4);
    if (( now_4 - last_time_away_report) > config.MinutesBetweenUploads ) {         
          time(&last_time_away_report);
          DEBUGPRINTLN1 ("away report this fact");

          int res = report_toCloud(-2);  
     
        }
// ------- report to cloud ----------------------------------


    
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

//---------------------------------------
//
