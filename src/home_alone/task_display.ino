/* -------------------------------------------------------------------
* Home Alone Application
* Based on a project presentd by Ralph Bacon
* This version used an ESP32 doing multitasking.
* by Peter B, from Switzerland
* Project website http://projects.descan.com/projekt7.html

---------------------------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------------------------
* 
* THIS Function RUNS as a separat task
* It handles the oled display
* that is all we do here !
* 
* -----------------------------------------------------
* keep this (not used):
*   display.setFont(ArialMT_Plain_24);
    display.drawString(0, 0, "Hello world");
    display.drawString(10,34, "whats up");
*/

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);


// Optionally include custom images
#include "images.h"

time_t display_on;
time_t now_1;
int update_oled;
char oled_buf[20];
String person2string;
bool display_state = false;

char state_display[] = { 'x', 'H', 'L', 'A', 'N', 'E', '\0' };

// LED stuff

const long interval = 1000;        // interval at which to blink (milliseconds)

unsigned long currentMillis = millis();
int greenledState = LOW;  // ledState used to set the LED

//-----------------------------------
void show_display() {
  // Serial.println (oled_buf);
  display.clearDisplay();
  display.display();  // actually display all of the above
  display.setFont();
  display.setTextColor(WHITE);
  display.setCursor(5, 1);
  display.println(" --> Home Alone <--");
  display.setFont(&FreeSans9pt7b);
  display.setTextColor(WHITE);
  display.setCursor(12, 30);

  //display.println("9  9  1  H 123");
  display.println(oled_buf);

  DEBUGPRINTLN1 ("Display ON");
  //    display.displayOn();
  display.display();  // actually display all of the above
  delay(200);
}


//
//---------------------------------------------------------
// function oled, runs as a sepatare task ------------
//----------------------------------------------------------
void task_display(void* parameter) {
  static long unsigned last_displaytime;
  static unsigned long lastEntryTime;
  static bool oled_first_time = true;
  int count;
  int count_day;


  for (;;) {  // do this forever....

    if (oled_first_time) {
      DEBUGPRINT1("\t\t\tTASK display - Running on core:");
      DEBUGPRINTLN1(xPortGetCoreID());
      oled_first_time = false;
      display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // Address 0x3C for 128x32
      delay(50);
      if (debug_flag > 0) config.ScreenTimeOutSeconds += 20;  // increase display on time if debug is on
      DEBUGPRINT1("\t\t\tTASK display -  - First done");
      // nothing else to do ....
    }  // end first time .......


    person2string = config.PersonName;

    // if another task wants to switch oled on, the oled signal will be set to > 0
    xSemaphoreTake(SemaOledSignal, portMAX_DELAY);  // signal oled task to switch display on
    if (oledsignal > NOSHOW) {
    update_oled = oledsignal;                       // take over the value
    oledsignal = NOSHOW;
    }
    xSemaphoreGive(SemaOledSignal);

  
    // what do we need to display?
    
    switch (update_oled) {
      
      case NORMAL:
        sprintf(oled_buf, "%d %d %d %c  %d", value1_oled, value2_oled, value3_oled, state_display[value4_oled], movCount_reportingPeriod_cloud);
        break;
      case ZEIT:
        sprintf(oled_buf, "%s", "WARTE ZEIT ");
        break;
      case WIFI:
        sprintf(oled_buf, "%s", "WARTE WIFI");
        break;
      case REBOOT:
        sprintf(oled_buf, "%s", "REBOOT");
        break;
      case START:
        sprintf(oled_buf, "%s", "START");
        break;
      case NOSHOW:
        sprintf(oled_buf, "%s", "NONO");
        break;

      default:
        DEBUGPRINTLN1("\t\t\t\t\tERROR display wrong order");
        sprintf(oled_buf, "%s", "??????");

    }  // end case statement -----------------------


    // DEBUGPRINTLN1(person2string);
    if (update_oled > NOSHOW) {

      show_display();
      time(&display_on);
      display_state = true;
      update_oled = NOSHOW;
    }

    if (display_state) {  // if display is on
      time(&now_1);
      if ((now_1 - display_on) > config.ScreenTimeOutSeconds) {
        DEBUGPRINTLN1 ("Display OFF");
        display.clearDisplay();
        display.display();
        display_state = false;
      }
    } else {
      // if anzeige aus ist
      //  read oled pin: presses means: switch display on for a number of seconds -----------
      int button_A = digitalRead(button_oledPin);  // read input value

      if (button_A == LOW) {  // check if the input is LOW
        DEBUGPRINTLN1("\t\t\tTASK display - button oled pressed");
        update_oled = NORMAL;  // verlange anzeige einschalten
      }
    }
    // blink green led is done here since display task is the first to run
    // blink green LED -------------------------
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;


      // if the LED is off turn it on and vice-versa:
      if (greenledState == LOW) {
        greenledState = HIGH;
        // set the LED with the ledState of the variable:
        digitalWrite(greenledPin, greenledState);
        delay(50);
        greenledState = LOW;
        digitalWrite(greenledPin, greenledState);
      }


      // Pause the task for 100ms
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}  // end endless loop
//---------------------------------------------------
// end of code
//---------------------------------------------------
