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
      DEBUGPRINT3 ("TASK detect - Running on core:");
      DEBUGPRINTLN3 (xPortGetCoreID());
      detect_first_time = false;
      timelastMovement = now;                            // Initiate time last movement
       // nothing else to do ....
    }
 
    
    val = digitalRead(inputPin); // read input value
    if (val == HIGH) { // check if the input is HIGH
      digitalWrite(redledPin, HIGH); // turn LED ON
   
      if (pirState == LOW) {
// we have just turned on
       DEBUGPRINTLN2 ("Motion detected!");
// We only want to print on the output change, not state
        pirState = HIGH;
      }
    } 
    else {
      digitalWrite(redledPin, LOW); // turn LED OFF
      if (pirState == HIGH){
// we have just turned of
       DEBUGPRINTLN2 ("Motion ended!");
   
        
// We only want to print on the output change, not state
        pirState = LOW;

        xSemaphoreTake(SemaMovement, portMAX_DELAY);
        ++movement_count;
        ++ movement_count_perday;
        if (movement_count > config.MaxActivityCount) movement_count = config.MaxActivityCount;
 
         count = movement_count;
         count_day = movement_count_perday;
         timelastMovement = now;
         
         xSemaphoreGive(SemaMovement);

        DEBUGPRINT2 ("count now: ");  DEBUGPRINT2 (count); DEBUGPRINT2 (" / ");  DEBUGPRINT2 (count_day); DEBUGPRINT2 (" - ");  DEBUGPRINTLN2 (timelastMovement);

      }
    }


  // Pause the task for 500ms
    vTaskDelay(500 / portTICK_PERIOD_MS);

  }
  } 

// end of code 
//---------------------------------------------------
