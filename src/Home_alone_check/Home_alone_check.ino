/*
 * 
 * OLED Feather (wie in Home Alone)
 * 
 * 
 */
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"

// Optionally include custom images
#include "images.h"
#define SDA 23
#define SCL 22

// Initialize the OLED display using Arduino Wire:
SSD1306Wire display(0x3c, SDA, SCL);   // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h


// LED stuff
unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 1000;           // interval at which to blink (milliseconds)
unsigned long currentMillis = millis();
int greenledState = LOW;             // ledState used to set the LED
int redledState = LOW;             // ledState used to set the LED

//---- Input-Output (gpio) related definitions and variables ----
#define redledPin 27        // choose the pin for the LED
#define greenledPin 12     // choose the pin for the LED
#define inputPin 33        // choose the input pin (for PIR sensor)
#define button_awayPin 14        // choose the input pin  (A)
#define button_oledPin 15        // choose the input pin  (C)
#define button_test 32        // choose the input pin  (C)



//------------------------------------------------
// displays at startup the Sketch running in the Arduino
void display_Running_Sketch (void){
  String the_path = __FILE__;
  int slash_loc = the_path.lastIndexOf('/');
  String the_cpp_name = the_path.substring(slash_loc+1);
  int dot_loc = the_cpp_name.lastIndexOf('.');
  String the_sketchname = the_cpp_name.substring(0, dot_loc);

  Serial.print("\n************************************");
  Serial.print("\nRunning Sketch: ");
  Serial.println(the_sketchname);
  Serial.print("Compiled on: ");
  Serial.print(__DATE__);
  Serial.print(" at ");
  Serial.print(__TIME__);
  Serial.print("\n");
  Serial.print("************************************\n");
}

//--------------------------------------------------------
void drawImageDemo() {
    // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
    // on how to create xbm files
    display.drawXbm(34, 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}


//-------------------------------------------------
void setup() {
 Serial.begin(115200);
  delay(2000);   //wait so we see everything
  display_Running_Sketch();

  pinMode(redledPin, OUTPUT);             // declare LED as output
  pinMode(greenledPin, OUTPUT);           // declare LED as output
  pinMode(inputPin, INPUT);               // declare as input
  pinMode(button_awayPin, INPUT_PULLUP);  // declare button as input
  pinMode(button_oledPin, INPUT_PULLUP);  // declare button as input
  pinMode(button_test, INPUT_PULLUP);  // declare button as input


         // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_24);
  drawImageDemo();
  display.display();
  delay(4000);
  display.clear();

    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 0, "Home Alone");

      display.drawString(0,34, "9  9  9 45 4");
 /*
//  display.setTextSize(1);
//  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Motion detect");
  display.println("okokok");
  display.println("Zeile 3");
  display.println("Zeile 4 abc");
  display.setCursor(0,0);

  */
      display.displayOn();
  display.display(); // actually display all of the above
  digitalWrite(redledPin, HIGH); // turn LED ON
  delay(300);
   digitalWrite(redledPin, LOW); // turn LED ON
  Serial.println("Setup done...");
}

void loop() {

  if(!digitalRead(button_awayPin)) Serial.println("Button_Away_C");
 
  if(!digitalRead(button_oledPin)) Serial.println("Button_oled_A");
  if(!digitalRead(button_test)) Serial.println("Button_Push_B");

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
    int val = digitalRead(inputPin); // read input value
    if (val == HIGH) { // check if the input is HIGH
      digitalWrite(redledPin, HIGH); 
    } // turn LED ON
    else      digitalWrite(redledPin, LOW); // turn LED ON
 
  delay(200);
  
  yield();
}
