/*
* Home Alone Application
* Based on a project presentd by Ralph Bacon

* This version based on a state machine
* 
* THIS RUNS as a separat task
* the only functio ist to detect movement and increment the movement counter
* that is all we do here !
* 
* 
*/

//---------------------------------------------------------------
// this function runs as a separate task to detect movement
//---------------------------------------------------------------
void task_detect ( void * parameter )
{
     static bool detect_first_time = true ;
      int count;
 for (;;) {
  

    if (detect_first_time) {
      DEBUGPRINT1 ("TASK detect - Running on core:");
      DEBUGPRINTLN1 (xPortGetCoreID());
      detect_first_time = false;
       // nothing else to do ....
    }
 
    
    val = digitalRead(inputPin); // read input value
    if (val == HIGH) { // check if the input is HIGH
      digitalWrite(redledPin, HIGH); // turn LED ON
   
      if (pirState == LOW) {
// we have just turned on
       DEBUGPRINTLN1 ("Motion detected!");
// We only want to print on the output change, not state
        pirState = HIGH;
      }
    } 
    else {
      digitalWrite(redledPin, LOW); // turn LED OFF
      if (pirState == HIGH){
// we have just turned of
       DEBUGPRINTLN1 ("Motion ended!");
   
        
// We only want to print on the output change, not state
        pirState = LOW;

        xSemaphoreTake(SemaMovement, portMAX_DELAY);
        ++movement_count;
        if (movement_count > config.MaxActivityCount) movement_count = config.MaxActivityCount;
        count = movement_count;
        movement_count_perday = movement_count_perday + count;
        xSemaphoreGive(SemaMovement);

       DEBUGPRINT1 ("count now: ");
       DEBUGPRINTLN1 (count);

      }
    }

    int but = digitalRead(button_awayPin); // read input value
    if (but == LOW) { // check if the input is HIGH
     DEBUGPRINTLN1 ("button away pressed");
      xSemaphoreTake(SemaButton, portMAX_DELAY);
        button_awaypressed = 1;
      xSemaphoreGive(SemaButton);
      
 
    }

  // Pause the task for 500ms
    vTaskDelay(500 / portTICK_PERIOD_MS);

  }
  } 

// end of code 
//---------------------------------------------------
