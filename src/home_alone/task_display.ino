/* -------------------------------------------------------------------
* Home Alone Application
* Based on a project presentd by Ralph Bacon
* This version used an ESP32 doing multitasking.
* by Peter B, from Switzerland
* Project website http://projects.descan.com/projekt7.html

---------------------------------------------------------------------------------------------------------------
Attention:

  I tried several versions of oled libs, including the one from Adafruit. There seem to be timing problems with the ESP32. See the web - they all
  work well with ESP8266 but not with ESP32.
  Finally I found this lib to work ok, I followed the following description:
  https://techtutorialsx.com/2017/12/02/esp32-arduino-interacting-with-a-ssd1306-oled-display/

------------------------------------------------------------------------------------------------------------
* 
* THIS Function RUNS as a separat task
* It hanldes the oled display
* that is all we do here !
* 
* -----------------------------------------------------
* keep this:
*   display.setFont(ArialMT_Plain_24);
    display.drawString(0, 0, "Hello world");
    display.drawString(10,34, "whats up");
*/

#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"

// Optionally include custom images
#include "images.h"
#define SDA 23
#define SCL 22

// Initialize the OLED display using Arduino Wire:
SSD1306Wire display(0x3c, SDA, SCL);   // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h


time_t display_on;
time_t now_1;
static volatile int update_oled;
char oled_buf[20];

char state_display[] = {'x', 'H', 'L', 'A', 'N', 'E', '\0'};

// LED stuff
unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 1000;           // interval at which to blink (milliseconds)
unsigned long currentMillis = millis();
int greenledState = LOW;             // ledState used to set the LED

//
//--------------------------------------------------------- 
// function oled, runs as a sepatare task ------------
//----------------------------------------------------------
void task_display ( void * parameter )
{ 
      static long unsigned last_displaytime;
      static unsigned long lastEntryTime;
      static bool oled_first_time = true ;
      static bool oled_on = false;
      int count;
      int count_day;

      
 for (;;) {                         // do this forever....
  
    if (oled_first_time) {
      DEBUGPRINT1 ("TASK display - Running on core:");
      DEBUGPRINTLN1 (xPortGetCoreID());
      oled_first_time = false;

       // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
      display.init();
      display.flipScreenVertically();
      display.setFont(ArialMT_Plain_24);
       drawImageDemo();
       display.display();
       delay(4000);
       display.clear();
       update_oled = 1;
       if (debug_flag > 0) config.ScreenTimeOutSeconds  += 20;     // increase display on time if debug is on 
      // nothing else to do ....
    }

    xSemaphoreTake(SemaOledSignal, portMAX_DELAY);    // signal oled task to switch display on
    if (oledsignal > 0)  update_oled = 1;
    oledsignal = 0;
    xSemaphoreGive(SemaOledSignal);

  //  Serial.print("oled-update: ");
  //  Serial.println (update_oled);

    
    if (update_oled > 0)  {
      DEBUGPRINTLN2("oled_display ");
      
    // assemble string to be displayed   
      sprintf( oled_buf , "%d  %d  %d  %c %d", value1_oled, value2_oled, value3_oled, state_display[value4_oled], movement_count);

   // Serial.println (oled_buf);
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_24);
      display.drawString(0, 0, "Home Alone");

      display.drawString(0,34, oled_buf);
      
      time(&display_on);                  // time at which oled is switched on
      display.displayOn();
      display.display();                  // actually display all of the above
      oled_on= true;
      update_oled = 0;
    }

    
    time(&now_1);
    if (( now_1 - display_on) > config.ScreenTimeOutSeconds ) {
      display.displayOff();     // switch off oled display
      oled_on = false;
    }

  //  read ole pin: presses means: switch display on for a number of seconds -----------
    int but = digitalRead(button_oledPin); // read input value
  
    if ((but == LOW) and (oled_on == false)) { // check if the input is LOW
     DEBUGPRINTLN1 ("button oled pressed");
     oledsignal = 1;
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
    } else {
      greenledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(greenledPin, greenledState);
  }

    
 
  // Pause the task for 100ms
    vTaskDelay(100 / portTICK_PERIOD_MS);

  }
}
  
//--------------------------------------------------------
void drawImageDemo() {
    // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
    // on how to create xbm files
    display.drawXbm(34, 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}

// end of code
//---------------------------------------------------
