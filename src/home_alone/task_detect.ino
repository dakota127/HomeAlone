/* -------------------------------------------------------------------
* Home Alone Application
* Based on a project presentd by Ralph Bacon
* This version used an ESP32 doing multitasking.
* by Peter B, from Switzerland
* Project website http://projects.descan.com/projekt7.html
* 
*/

//---------------------------------------------------------------
// this function runs as a separate task to detect movement
//---------------------------------------------------------------
void task_detect ( void * parameter )
{
     static bool detect_first_time = true ;
     int count;
     int count_day;

 for (;;) {
  

    if (detect_first_time) {
      DEBUGPRINT2 ("TASK detect - Running on core:");
      DEBUGPRINTLN2 (xPortGetCoreID());
      detect_first_time = false;
      timelastMovement = now;                            // Initiate time last movement
       // nothing else to do ....
       // Pause the task 
      vTaskDelay(200 / portTICK_PERIOD_MS); 
    }
 
    
    val = digitalRead(inputPin);      // read input value
    if (val == HIGH) {                // check if the input is HIGH 
       if (pirState == LOW) {
          digitalWrite(redledPin, HIGH);  // turn LED ON
          DEBUGPRINTLN2 ("Motion detected!");
          pirState = HIGH;
        }
    }
    
    else {                            // input is low
      if (pirState == HIGH) {
        digitalWrite(redledPin, LOW); // turn LED OFF
        DEBUGPRINTLN2 ("Motion ended!");   
        pirState = LOW;

      // motion has ended, count this movement
        xSemaphoreTake(SemaMovement, portMAX_DELAY);
         ++movCount_reportingPeriod_cloud;
         ++movCount_reportingPeriod_push;
        if (movCount_reportingPeriod_cloud > config.MaxActivityCount) movCount_reportingPeriod_cloud = config.MaxActivityCount;
         count = movCount_reportingPeriod_cloud;
         timelastMovement = now;
         
         xSemaphoreGive(SemaMovement);

        DEBUGPRINT2 ("count now: ");  DEBUGPRINT2 (count); DEBUGPRINT2 (" / ");  DEBUGPRINT2 (count); DEBUGPRINT2 (" - ");  DEBUGPRINTLN2 (timelastMovement);

      }
    }


  // Pause the task for 500ms
    vTaskDelay(500 / portTICK_PERIOD_MS);

  }
  } 

// end of code 
//---------------------------------------------------
