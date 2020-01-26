/*
* Home Alone Application
* Based on a project presentd by Ralph Bacon

* This version based on a state machine
* 
* THIS function RUNS in TASK state_machine !
* ATHOME is one of the states of the machine
* we are in this state if somebody is home
* 
* We report to the cloud every n minutes
* wen check the 'I am leaving' pushbutton, if prssed, the machine changes to the "LEAVING" state
* otherwise the machine stays in the ATHOME state. 
* 
*/

int ath_count;

static bool athome_first_time = true;                // first_time run through a state, bitwise, bit 0 means state 1

int butto;

static time_t last_time_report;
static time_t last_time_push;
time_t now_2;

//---------------------------------------------------------------
// this function handles the state 'athome' from the state machine
//---------------------------------------------------------------
void do_athome() {


    if (athome_first_time) {
      DEBUGPRINT1("STATE athome - Running on core:");
      DEBUGPRINTLN1(xPortGetCoreID());
      time(&last_time_report);
      time(&last_time_push);
      athome_first_time = false;
      value4_oled = 1;              // signal is at home
      xSemaphoreTake(SemaOledSignal, portMAX_DELAY);    // signal oled task to switch display on
      oledsignal = 1;
      xSemaphoreGive(SemaOledSignal);
      
   
      DEBUGPRINT0 ("Upload intervall (sec): ");
      DEBUGPRINTLN0 (config.MinutesBetweenUploads);
      DEBUGPRINT0 ("Mail intervall (sec): ");
      DEBUGPRINTLN0 (config.MinutesBetweenEmails);
      DEBUGPRINT0 ("Push intervall (sec): ");
      DEBUGPRINTLN0 (config.MinutesBetweenPushover);
       // nothing else to do ....
    }
 

 // ------- report to cloud ----------------------------------
    time(&now_2);
    if (( now_2 - last_time_report) > config.MinutesBetweenUploads ) {         
          time(&last_time_report);
          xSemaphoreTake(SemaMovement, portMAX_DELAY);
          ath_count = movement_count;
          xSemaphoreGive(SemaMovement);


       DEBUGPRINTLN1 ("trying to report to cloud");
 // we need to report to the cloud - this is done in the wifi task
 // first we need to se if task is free or busy - we check the tasks semaphore
 
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait 10 ticks to see if it becomes free. */
        if( xSemaphoreTake( wifi_semaphore, ( TickType_t ) 100 ) == pdTRUE )    //pdFALSE auch gültig
        {
            /* We were able to obtain the semaphore and can now access the
            shared resource. */
         // set up parameter for this job
            wifi_todo = REPORT_CLOUD;
            wifi_order_struct.order = wifi_todo;
            wifi_order_struct.mvcount  = movement_count;
            vTaskResume( Task1 );
            /* We have finished accessing the shared resource.  Release the
            semaphore. 
            No: the wifi task is freeing it */
         //   xSemaphoreGive( wifi_semaphore );
        }
        else
        {
            /* We could not obtain the semaphore and can therefore not access
            the shared resource safely. */

            Serial.println ("wifi semaphore busy (cloud reporting)");
            vTaskDelay(200 / portTICK_PERIOD_MS);

        }
    

 
  vTaskDelay(200 / portTICK_PERIOD_MS);

  int res =0;
          if (res == 0) {           // reset count if ok 
            xSemaphoreTake(SemaMovement, portMAX_DELAY);
            movement_count = 0;
            xSemaphoreGive(SemaMovement);
          }
// ------- report to cloud ----------------------------------
     
    }  // end Zeit erreicht




// check if button I am leaving is pressed --------------
     xSemaphoreTake(SemaButton, portMAX_DELAY);
     butto= button_awaypressed;
     xSemaphoreGive(SemaButton);

      if (butto >0) {
        state= LEAVING;
        athome_first_time = true;  
      }

//   char time_output[30];
//    strftime(time_output, 30, "%a  %d-%m-%y %T", localtime(&now2));

    int but = digitalRead(button_test); // read input value
    if (but == LOW) { // check if the input is HIGH
     DEBUGPRINTLN1 ("button test pressed");
    
      test_push();
 
    }



    time(&now);
    localtime_r (&now, &timeinfo);
  
    int currenthour = timeinfo.tm_hour;


   
    if (currenthour >= config.QuietHoursStart) {
        state= NIGHT;
        athome_first_time = true;   
        old_dayofyear = timeinfo.tm_yday;
        old_year = timeinfo.tm_year -100;   // function returns year since 1900
        
        DEBUGPRINTLN1 ("Good night...");
    }
  //  Serial.print ("Stunde: "); Serial.println (timeinfo.tm_hour);
    vTaskDelay(500 / portTICK_PERIOD_MS);
//  DEBUGPRINTLN1 ("do_athome()");


}



void test_push() {
  
// we need to report to the cloud - this is done in the wifi task
 // first we need to se if task is free or busy - we check the tasks semaphore

  
  
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait 10 ticks to see if it becomes free. */
        if( xSemaphoreTake( wifi_semaphore, ( TickType_t ) 100 ) == pdTRUE )
        {
            /* We were able to obtain the semaphore and can now access the
            shared resource. */
         // set up parameter for this job
            wifi_todo = PUSH_MESG;
            wifi_order_struct.order = wifi_todo;
            wifi_order_struct.pushtext = "Test Message";
              
            Serial.println (wifi_order_struct.pushtext);
            vTaskResume( Task1 );
            /* We have finished accessing the shared resource.  Release the
            semaphore. */
    //        xSemaphoreGive( wifi_semaphore );
        }
        else
        {
            /* We could not obtain the semaphore and can therefore not access
            the shared resource safely. */

            Serial.println ("wifi semaphore busy (push test msg)");
            vTaskDelay(200 / portTICK_PERIOD_MS);

        }
}      

// end of cde -----------------------------------
