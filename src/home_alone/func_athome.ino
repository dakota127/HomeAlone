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
int ath_count_day;

static bool athome_first_time = true;                // first_time run through a state, bitwise, bit 0 means state 1

bool do_report= false;


time_t now_2;

//---------------------------------------------------------------
// this function handles the state 'athome' from the state machine
//---------------------------------------------------------------
void do_athome() {


    if (athome_first_time) {
      DEBUGPRINT1("STATE athome - Running on core:");
      DEBUGPRINTLN1(xPortGetCoreID());
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
 

  // get semaphore and read the clock number 1 ------------------
   if( xSemaphoreTake( clock_1Semaphore, ( TickType_t ) 10 ) == pdTRUE )  {     // semaphore obtained, now do the work
       if (clock_tick_1) {
           DEBUGPRINTLN1  ("Clock 1 ticked....");
           clock_tick_1 = false;
           do_report = true;
           }
       vTaskDelay(10 / portTICK_PERIOD_MS);
       xSemaphoreGive( clock_1Semaphore );              // release semaphore

// get movement count ----------
       xSemaphoreTake(SemaMovement, portMAX_DELAY);
       ath_count = movement_count;
       ath_count_day = movement_count_perday;
       xSemaphoreGive(SemaMovement);

   }
    else  {                                             // semaphore busy, do nothing....
       DEBUGPRINTLN1 ("Semaphore clock 1busy");
    }

    if (do_report) {
      DEBUGPRINT1 ("trying to report to cloud, count:  ");   DEBUGPRINT1 (ath_count); DEBUGPRINT1 (" / "); DEBUGPRINTLN1 (ath_count_day); 
       do_report = false;
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
            wifi_order_struct.mvcount  = ath_count;
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

            DEBUGPRINTLN1  ("wifi semaphore busy (cloud reporting)");
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
    }



// check if button I am leaving is pressed --------------
     xSemaphoreTake(SemaButton, portMAX_DELAY);
     int butto = button_awaypressed;
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
              
            DEBUGPRINTLN1 (wifi_order_struct.pushtext);
            vTaskResume( Task1 );
            /* We have finished accessing the shared resource.  Release the
            semaphore. */
    //        xSemaphoreGive( wifi_semaphore );
        }
        else
        {
            /* We could not obtain the semaphore and can therefore not access
            the shared resource safely. */

            DEBUGPRINTLN1  ("wifi semaphore busy (push test msg)");
            vTaskDelay(200 / portTICK_PERIOD_MS);

        }
}      

// end of cde -----------------------------------
