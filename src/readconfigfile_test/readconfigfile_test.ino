/*
  SD card read

 To verrify the sd card and the config.json file

 */

#include <SD.h>

File myFile;



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


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  delay(2000);   //wait so we see everything
  display_Running_Sketch();

 Serial.println("Read config.json file on SD Card");

  Serial.println("Initializing SD card...");

  if (!SD.begin(33)) {
    Serial.println("SD initialization failed!");
    while (1);
  }
  Serial.println("SD initialization done.");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
 


  // re-open the file for reading:
  myFile = SD.open("/config.json", FILE_READ);
  if (myFile) {
    Serial.println("Inhalt config.json:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening file");
  }

  Serial.println (" ");
  Serial.println (" ");
  Serial.println ("done...");
}

void loop() {
  // nothing happens after setup
}
