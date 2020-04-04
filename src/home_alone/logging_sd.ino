/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
https://randomnerdtutorials.com/esp32-data-logging-temperature-to-microsd-card/

22.3.2020  Peter
  
*********/


 const int chipSelect = 33;
 bool failed = false;

//------------------------------------------------------
int logInit(const char * path) {

  int anz_try = 0;
  
    DEBUGPRINT1 ("->doing logInit()  path: ");  DEBUGPRINTLN1 (path); 
  // Initialize SD card
    while (!SD.begin(chipSelect))  { 
         anz_try++;
         if (anz_try > 40) // try again 
          { 
            DEBUGPRINTLN0 ("\nFailed to initialize SD library");
            failed = true;
            delay(3000);
            ESP.restart();
     
          }
          delay(100);  
          DEBUGPRINT0 ("."); 
        }
  

  // If the data.txt file doesn't exist
  // Create a file on the SD card and write the data labels
  File file = SD.open(path);
  if(!file) {
    DEBUGPRINT2 ("File doesn't exist, creating file: "); DEBUGPRINTLN2 (path);
    writeFile(SD, path, "Reading ID, Date, Hour, Temperature \r\n");
  }
  else {
    DEBUGPRINTLN2 ("File already exists on sd-card");  
  }
  file.close();
}
//---------------------------------------------------------

 
//------------------------------------------------------
// Write the sensor readings on the SD card
void log_SDCard(String logEntry,const char * path) {
  DEBUGPRINTLN1 ("->doing log_SDCard()"); 
  DEBUGPRINT1 ("message: ");  DEBUGPRINT1 (logEntry); 

  appendFile(SD, path, logEntry.c_str());
}

//---------------------------------------------------------
// Write to the SD card (DON'T MODIFY THIS FUNCTION)
// initla creation of logfile
void writeFile(fs::FS &fs, const char * path, const char * message) {
  DEBUGPRINT1 ("->doing writeFile(), file: "); DEBUGPRINTLN1 (path); 
  DEBUGPRINT1 ("message: "); DEBUGPRINTLN1 (message); 
  

  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    DEBUGPRINTLN0 ("Failed to open file for writing");
    return;
  }
  if(file.print(message)) {
    DEBUGPRINTLN2 ("new logfile written");
  } else {
    DEBUGPRINTLN0 ("Write failed");
  }
  file.close();
}

//--------------------------------------------------------------------
// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char * path, const char * message) {

  DEBUGPRINTLN2 ("->doing appendFile()");

  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    DEBUGPRINTLN0  ("Failed to open file for appending");
    return;
  }
  if(file.print(message)) {
    DEBUGPRINTLN2 ("Message appended");
  } else {
    DEBUGPRINTLN0 ("Append failed");
  }
  file.close();
}
//-------------------------------------------------------
