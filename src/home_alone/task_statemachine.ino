/* -------------------------------------------------------------------
  Home Alone Application
  Based on a project presentd by Ralph Bacon
  This version used an ESP32 doing multitasking.
  by Peter B, from Switzerland
  Project website http://projects.descan.com/projekt7.html

  THIS Function RUNS as a separat task
  It implements the state machine

  -------------------------------------------------------------------
*/


static bool away_first_time = true;     // first_time run through a state, bitwise, bit 0 means state 1
static bool athome_first_time = true;   // first_time run through a state, bitwise, bit 0 means state 1
static bool leaving_first_time = true;  // first_time run through a state, bitwise, bit 0 means state 1

int movCount_counter_1_temp;
int movCount_counter_2_temp;
bool do_report = false;
int number_of_tries_eveningreporting = 0;


time_t leaving_start;  // leave state leaving after n min
time_t now_3;          // time for leaving
#define WAIT_TO_REPORT 10000

char bufpush[80];

int retcode;
int timelastmv;

//-----------------------------------------------------------
//-----------------------------------------------------------
// implementation of the state machine (Runs in separate task !)
//-----------------------------------------------------------
//-----------------------------------------------------------
void state_machine(void* parameter) {
  static bool main_firsttime = true;

  for (;;) {

    if (main_firsttime) {
      DEBUGPRINT2("\t\tTASK state_machine - Running on core:");
      DEBUGPRINTLN2(xPortGetCoreID());
      main_firsttime = false;
      state = ATHOME;  // initial state of state machine
    }

    // ---------------------------------------
    // stuff we do irrespective of the state --------

    // get current date info --------
    time(&aktuelle_epochzeit);
    localtime_r(&aktuelle_epochzeit, &timeinfo);

    curr_hour = timeinfo.tm_hour;  // used for morning/evening reporting

    // get semaphore and read the clock number 1 ------------------
    xSemaphoreTake(clock_1Semaphore, portMAX_DELAY);
    if (clock_tick_1) {
      DEBUGPRINTLN1("\t\tTASK state_machine away Clock 1 ticked....");
      clock_tick_1 = false;
      do_report = true;
      // get movement count and time last movement ----------
      xSemaphoreTake(SemaMovement, portMAX_DELAY);
      movCount_counter_1_temp = movCount_counter_1;
      xSemaphoreGive(SemaMovement);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
    xSemaphoreGive(clock_1Semaphore);  // release semaphore


if (do_report) {

    // cap number of movements
    if (movCount_counter_1_temp > config.MaxActivityCount) movCount_counter_1_temp = config.MaxActivityCount;
    DEBUGPRINT1("\t\tTASK state_machine trying to report to thingspeak, count:  ");
    DEBUGPRINTLN1(movCount_counter_1_temp);

    do_report = false;
    // set up parameter for this job
    wifi_todo = THINGSPEAK;
    wifi_order_struct.order = wifi_todo;
    wifi_order_struct.mvcount = movCount_counter_1_temp;
    retcode = wifi_func();
    DEBUGPRINT2("\t\tTASK state_machine wifi_func returns: ");
    DEBUGPRINTLN2(retcode);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    if (retcode == 0) {  // reset count if ok
      xSemaphoreTake(SemaMovement, portMAX_DELAY);
      movCount_counter_1 = 0;
      xSemaphoreGive(SemaMovement);
    }
}
//  if no movements during the last 24 hours -> send pushover msg
  if ((aktuelle_epochzeit - timelastMovement) >= (config.HoursbetweenNoMovementRep * 3600 * alert_reporting_counter)) {
    ++alert_reporting_counter;
    DEBUGPRINTLN1("\t\tTASK state_machine alert message (no movements)");
    // assemble string to be displayed
    sprintf(bufpush, "Keine Bewegung in den letzten %d Stunden", config.HoursbetweenNoMovementRep);

    // push alert message ------------
    retcode = push_msg(bufpush, 1);  // High priority message  in red
    DEBUGPRINT1("\t\tTASK state_machine push_msg() returns: ");
    DEBUGPRINTLN1(retcode);
  }

// end of general stuff that we do in all states
//-----------------------------------------------
//
// state machine implementation starts here ----------------------
// switch according to state variable

    vTaskDelay(200 / portTICK_PERIOD_MS);
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

      default:
        DEBUGPRINTLN0("\t\tTASK state_machine - case main zu default, error!");
        vTaskDelay(200 / portTICK_PERIOD_MS);
        break;
    }  // end case stmt

    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}



//---------------------------------------------------------------
//----------------------------------------------------------------
// this function handles the state 'athome' from the state machine
//---------------------------------------------------------------
//---------------------------------------------------------------
void do_athome() {


  if (athome_first_time) {
    DEBUGPRINTLN2("\t\tTASK state_machine - STATE athome");
    athome_first_time = false;
    value4_oled = 1;                                // signal is at home
    xSemaphoreTake(SemaOledSignal, portMAX_DELAY);  // signal oled task to switch display on
    oledsignal = NORMAL;
    xSemaphoreGive(SemaOledSignal);

    // nothing else to do ....
  }


  DEBUGPRINTLN3("in state ATHOME.....");

  // check button c press
  int but = digitalRead(button_awayPin);  // read input value leaving button
  if (but == LOW) {                       // check if the input is HIGH
    DEBUGPRINTLN1("\t\tTASK state_machine button away pressed");
    state = LEAVING;
    athome_first_time = true;
  }

  // check if test button is pressed, send pushmesage if so....--------
  but = digitalRead(button_test);  // read input value test button, send test message
  if (but == LOW) {                // check if the input is HIGH
    DEBUGPRINTLN1("\t\tTASK state_machine button test pressed");

    // get movement count
    xSemaphoreTake(SemaMovement, portMAX_DELAY);
    movCount_counter_2_temp = movCount_counter_2;  // HOLE TOTAL ANZAHL MOVEMENTS
    xSemaphoreGive(SemaMovement);

    sprintf(bufpush, "Testmeldung, Last Reset: %s count: %d   ", time_lastreset, movCount_counter_2_temp);
    retcode = push_msg(bufpush, 1);  // use priority 1 HIGH
    DEBUGPRINT1("\t\tTASK state_machine push_msg() returns: ");
    DEBUGPRINTLN1(retcode);
    vTaskDelay(400 / portTICK_PERIOD_MS);
  }



/*
  if ((aktuelle_epochzeit - timelastmv) >= (config.HoursbetweenNoMovementRep * 3600)) {
    DEBUGPRINTLN1("\t\tTASK state_machine no movement for the last period");
    // assemble string to be displayed
    sprintf(bufpush, "Keine Bewegung in den letzten %d Stunden", config.HoursbetweenNoMovementRep);

    // push alert message ------------
    retcode = push_msg(bufpush, 1);  // High priority message  in red
    DEBUGPRINT1("\t\tTASK state_machine push_msg() returns: ");
    DEBUGPRINTLN1(retcode);
    xSemaphoreTake(SemaMovement, portMAX_DELAY);
    timelastMovement = aktuelle_epochzeit;
    xSemaphoreGive(SemaMovement);
  }
*/

  // have we reached evening reporting hour yet ?
  if (config.EveningReportingHour > 0) {
    eveningReporting();  // do it
  }

  vTaskDelay(200 / portTICK_PERIOD_MS);
  //  DEBUGPRINTLN1 ("do_athome(


}  // end do athome -----


//---------------------------------------------------------------
//----------------------------------------------------------------
// this function handles the state 'away' from the state machine
//---------------------------------------------------------------
//---------------------------------------------------------------
void do_away() {

  if (away_first_time) {
    DEBUGPRINTLN2("\t\tTASK state_machine STATE away");
    away_first_time = false;
    value4_oled = 3;                                // signal is away
    xSemaphoreTake(SemaOledSignal, portMAX_DELAY);  // signal oled task to switch display on
    oledsignal = NORMAL;
    xSemaphoreGive(SemaOledSignal);
  }
  DEBUGPRINTLN3("in state AWAY.....");

  // check if movement
  // if so we switch to state ATHOME ------------------
  // and send pushover that target is at home again
  xSemaphoreTake(SemaMovement, portMAX_DELAY);
  movCount_counter_1_temp = movCount_counter_1;
  xSemaphoreGive(SemaMovement);
  if (movCount_counter_1_temp > 0) {
    state = ATHOME;
    away_first_time = true;
    athome_first_time = true;
    DEBUGPRINTLN2("\t\tTASK state_machine away movement count > 0 leaving state away ");
    sprintf(bufpush, "Bewohner wieder daheim");
    retcode = push_msg(bufpush, 0);  // use priority 1 HIGH
    DEBUGPRINT1("\t\tTASK state_machine push_msg() returns: ");
    DEBUGPRINTLN1(retcode);
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
    DEBUGPRINTLN2("\t\tTASK state_machine STATE leaving");
    DEBUGPRINT2("\t\tTASK state_machine leaving_timeout (sec): ");
    DEBUGPRINTLN2(config.TimeOutLeavingSec);
    leaving_first_time = false;
    value4_oled = 2;                                // signal is at leaving
    xSemaphoreTake(SemaOledSignal, portMAX_DELAY);  // signal oled task to switch display on
    oledsignal = NORMAL;
    xSemaphoreGive(SemaOledSignal);

    time(&leaving_start);

    sprintf(bufpush, "Bewohner verlässt das Haus");
    retcode = push_msg(bufpush, 0);  // use priority 1 HIGH
    DEBUGPRINT1("\t\tTASK state_machine push_msg() returns: ");
    DEBUGPRINTLN1(retcode);
    xSemaphoreTake(SemaMovement, portMAX_DELAY);
    movCount_counter_1 = 0;
    movCount_counter_2 = 0;
    xSemaphoreGive(SemaMovement);

    vTaskDelay(400 / portTICK_PERIOD_MS);
  }

  DEBUGPRINTLN3("in state LEAVING.....");

  time(&now_3);

  if ((now_3 - leaving_start) > (config.TimeOutLeavingSec)) {  // ok, we leave this state
    //  Serial.println ( now_3 - leaving_start);
    xSemaphoreTake(SemaMovement, portMAX_DELAY);
    movCount_counter_1 = 0;
    xSemaphoreGive(SemaMovement);
    state = AWAY;
    away_first_time = true;
    leaving_first_time = true;
    DEBUGPRINTLN1("\t\tTASK state_machine end state leaving, next state: away");
  }

  vTaskDelay(1000 / portTICK_PERIOD_MS);

}  // end do_leaving ----------


// EVening Reporting -----------------------------------------
//---------------------------------------------------------------

void eveningReporting() {


  if (curr_hour < config.EveningReportingHour) done_eveningreporting = false;
  else {
    //    we are past reporting hour
    if (done_eveningreporting == false) {  // do evening reporting

      if (essential_debug) Serial.println("\t\tTASK state_machine Evening Reporting");

      // get movement count ----------
      xSemaphoreTake(SemaMovement, portMAX_DELAY);
      movCount_counter_2_temp = movCount_counter_2;
      xSemaphoreGive(SemaMovement);

      sprintf(bufpush, "Gute Nacht, Anzahl Bewegungen seit Gestern: %d   ", movCount_counter_2_temp);

      // push evening message ------------
      retcode = push_msg(bufpush, 0);  // prio zero
      DEBUGPRINT1("\t\tTASK state_machine push_msg() returns: ");
      DEBUGPRINTLN1(retcode);

      // check if push msg was ok. If not try one more time.
      // if still not ok, forget it and set done_eveningreporting to true.
      if (retcode == 0) {
        xSemaphoreTake(SemaMovement, portMAX_DELAY);
        movCount_counter_2 = 0;  // clear counter movements total
        xSemaphoreGive(SemaMovement);
        done_eveningreporting = true;
        number_of_tries_eveningreporting = 0;

      } else {  // evening reporting was not ok
        vTaskDelay(4000 / portTICK_PERIOD_MS);

        number_of_tries_eveningreporting += 1;       // repeat count evening reporting
        if (number_of_tries_eveningreporting > 2) {  // second tme not ok, forget it
          done_eveningreporting = true;
          number_of_tries_eveningreporting = 0;
        }
      }
    }
  }
  // else: no evening reorting necessary...
}


// end of code
//---------------------------------------------------
