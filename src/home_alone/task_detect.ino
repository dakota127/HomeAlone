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
void task_detect(void* parameter) {
  static bool detect_first_time = true;

  for (;;) {

    if (detect_first_time) {
      DEBUGPRINT2("\tTASK detect - Running on core:");
      DEBUGPRINTLN2(xPortGetCoreID());
      detect_first_time = false;
      timelastMovement = aktuelle_epochzeit;  // Initiate time last movement
                                              // nothing else to do ....
                                              // Pause the task
      vTaskDelay(200 / portTICK_PERIOD_MS);
    }


    val = digitalRead(sensorPin);  // read input value PIR sensor
    if (val == HIGH) {             // check if the input is HIGH
      if (pirState == LOW) {
        digitalWrite(redledPin, HIGH);  // turn LED ON
        DEBUGPRINTLN2("\tTASK detect - Movement detected!");
        pirState = HIGH;
      }
    }

    else {  // input is low
      if (pirState == HIGH) {
        digitalWrite(redledPin, LOW);  // turn LED OFF
        DEBUGPRINTLN2("\tTASK detect - Movement ended!");
        pirState = LOW;

        // motion has ended, count this movement counter protected by semaphore
        xSemaphoreTake(SemaMovement, portMAX_DELAY);
        ++movCount_reportingPeriod_cloud;  // thingsSpeak counter, we report a max. of MaxActivityCount Movements
        if (movCount_reportingPeriod_cloud > config.MaxActivityCount) movCount_reportingPeriod_cloud = config.MaxActivityCount;

        ++movCount_reportingPeriod_push;  // movement within Morning or evening reporting period
        timelastMovement = aktuelle_epochzeit;
        xSemaphoreGive(SemaMovement);


        if (essential_debug) {
          Serial.print("\tTASK detect - Movement count: ");
          Serial.print(movCount_reportingPeriod_cloud);
          Serial.print(" / ");
          Serial.print(movCount_reportingPeriod_push);
          Serial.print(" - ");
          Serial.println(timelastMovement);
        }
      }

      // Pause the task for 200ms
      vTaskDelay(200 / portTICK_PERIOD_MS);
    }
  }
}
  // end of code
  //---------------------------------------------------
