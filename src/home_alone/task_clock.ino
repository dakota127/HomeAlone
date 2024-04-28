/* -------------------------------------------------------------------
* Home Alone Application
* Based on a project presentd by Ralph Bacon
* This version used an ESP32 doing multitasking.
* by Peter B, from Switzerland
* Project website http://projects.descan.com/projekt7.html

*/



static bool done_clock_tick_1 = false;  // have we already done it at the specified hour ?
static bool done_clock_tick_2 = false;

static time_t last_time_clock_1;

char buf[20];
//
//---------------------------------------------------------
// function task_clock(), runs as a sepatare task ------------
//----------------------------------------------------------
void task_clock(void* parameter) {


  static bool clock_firsttime = true;
  for (;;) {

    // do first time through.......
    if (clock_firsttime) {
      DEBUGPRINT2("\tTASK clock - Running on core: ");
      DEBUGPRINTLN2(xPortGetCoreID());
      clock_firsttime = false;
      time(&last_time_clock_1);
      vTaskDelay(5000 / portTICK_PERIOD_MS);

    DEBUGPRINT1 ("\tTASK clock Upload intervall (sec): ");
    DEBUGPRINTLN1 (config.MinutesBetweenUploads);
    DEBUGPRINT1 ("\tTASK clock Msg after no movement (hours): ");
    DEBUGPRINTLN1 (config.HoursbetweenNoMovementRep);

      // nothing else to do ....
    }



     //---------------------------------------------------------------------------------------
    // -----  handle clock_1 (used for upload data to cloud (ThingSpeak))------------
    //---------------------------------------------------------------------------------------

    if (config.MinutesBetweenUploads > 0) {  // do reporting only if value is > zero

      if ((aktuelle_epochzeit - last_time_clock_1) > config.MinutesBetweenUploads ) {

        time(&last_time_clock_1);                                          // store time
                                                                           // get semaphore and set the clock -------------------
        if (xSemaphoreTake(clock_1Semaphore, (TickType_t)10) == pdTRUE) {  // semaphore obtained, now do the work
          DEBUGPRINTLN1("\tTASK clock setze clock_1");
          clock_tick_1 = true;
          vTaskDelay(10 / portTICK_PERIOD_MS);
          xSemaphoreGive(clock_1Semaphore);  // release semaphore
        } else {                             // semaphore busy, do nothing....
          Serial.println("\tTASK clock - Semaphore busy");
        }
      }
    }


    vTaskDelay(600 / portTICK_PERIOD_MS);
  }   // end endless loop
}  // end function



// end of code
//---------------------------------------------------
