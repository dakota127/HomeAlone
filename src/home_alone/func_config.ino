/* -------------------------------------------------------------------
* Home Alone Application
* Based on a project presentd by Ralph Bacon
* This version used an ESP32 doing multitasking.
* by Peter B, from Switzerland
* Project website http://projects.descan.com/projekt7.html
* 
* THIS Function RUNS in TASK state_machine !
* AWAY is one of the states of the machine
* we are in this state if nobody is home
* 
* We report this fact to the cloud every n minutes with field1 = -5
* upon detecting a motion the machine switches to the ATHOME state (no burglars are reported)
* if no movement the machine remains in this state
* 
* JSON:
* arduinojson.org/v6/assistant 
* to compute the capacity.
*/


//----------------------------------------------------------------
// Loads the configuration from a file
int loadConfig (const char *filename, Config &config) {

// Initialize SD library
  const int chipSelect = 33;
  int anz_try = 0;
  bool failed = false;

   DEBUGPRINT2 ("start load config, filename: ");  DEBUGPRINTLN1  (filename); 
   delay(300);
   
   while (!SD.begin(chipSelect))  { 
         anz_try++;
         if (anz_try > 40) // try again 
          { 
            DEBUGPRINTLN0 ("\nFailed to initialize SD library");
            failed = true;
           ESP.restart();
            return (9);
          }
          delay(100);  
         DEBUGPRINT2 ("."); 
        }
  
  
  // Open file for reading
  File file = SD.open("/config.json", FILE_READ);

  if (file == 1) Serial.print ("File opened: ");
  else Serial.print ("File not found: ");
  Serial.println(filename);

// Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.

   DynamicJsonDocument doc(1400);

 // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
}

 config.ThingSpeakChannelNo = doc["ThingSpeakChannelNo"]; // 
          
config.ThingSpeakFieldNo = doc["ThingSpeakFieldNo"]; // 1

 strlcpy(config.ThingSpeakWriteAPIKey,                  // <- destination
          doc["ThingSpeakWriteAPIKey"] | "example.ch",  // <- source
          sizeof(config.ThingSpeakWriteAPIKey));         // <- destination's capacity

 strlcpy(config.Title,                  // <- destination
          doc["Title"] | "Movements ",  // <- source
          sizeof(config.Title));         // <- destination's capacity          
          
 strlcpy(config.PersonName,                  // <- destination
          doc["PersonName"] | "Aunt Daisy",  // <- source
          sizeof(config.PersonName));         // <- destination's capacity
          
 strlcpy(config.PushoverUserkey,                  // <- destination
          doc["PushoverUserkey"] | "example.ch",  // <- source
          sizeof(config.PushoverUserkey));         // <- destination's capacity
           
 strlcpy(config.PushoverToken,                  // <- destination
          doc["PushoverToken"] | "example.ch",  // <- source
          sizeof(config.PushoverToken));         // <- destination's capacity
                             
config.MinutesBetweenUploads = doc["MinutesBetweenUploads"]; // 14
config.MinutesBetweenUploads = config.MinutesBetweenUploads * 60;
 strlcpy(config.wlanssid_1,                  // <- destination
          doc["SSID_1"] | "example.ch",  // <- source
          sizeof(config.wlanssid_1));         // <- destination's capacity

  credentials[0]=  config.wlanssid_1;

        
  strlcpy(config.wlanpw_1,                  // <- destination
          doc["Password_1"] | "example.ch",  // <- source
          sizeof(config.wlanpw_1));         // <- destination's capacity
  credentials[1]=  config.wlanpw_1;   

     
  strlcpy(config.wlanssid_2,                  // <- destination
          doc["SSID_2"] | "example.ch",  // <- source
          sizeof(config.wlanssid_2));         // <- destination's capacity
  credentials[2]=  config.wlanssid_2;    

     
  strlcpy(config.wlanpw_2,                  // <- destination
          doc["Password_2"] | "example.ch",  // <- source
          sizeof(config.wlanpw_2));         // <- destination's capacity
   credentials[3]=  config.wlanpw_2;        

  
  strlcpy(config.Timezone_Info,                  // <- destination
          doc["Timezone_Info"] | "example.ch",  // <- source
          sizeof(config.Timezone_Info));         // <- destination's capacity

  strlcpy(config.NTPPool,                  // <- destination
          doc["NTPpool"] | "example.ch",  // <- source
          sizeof(config.NTPPool));         // <- destination's capacity

  strlcpy(config.Email_1,                  // <- destination
          doc["Email1"] | "example.ch",  // <- source
          sizeof(config.Email_1));         // <- destination's capacity

  strlcpy(config.Email_2,                  // <- destination
          doc["Email2"] | "example.ch",  // <- source
          sizeof(config.Email_2));         // <- destination's capacity

  strlcpy(config.PushoverDevices,                  // <- destination
          doc["PushoverDevices"] | "example.ch",  // <- source
          sizeof(config.PushoverDevices));         // <- destination's capacity
  



config.TimeOutLeavingSec = doc["TimeOutLeavingSec"]; // 30
config.MaxActivityCount = doc["MaxActivityCount"]; // 25
config.ScreenTimeOutSeconds = doc["ScreenTimeOutSeconds"]; // 30

config.EveningReportingHour = doc["EveningReportingHour"]; // 30
config.MorningReportingHour = doc["MorningReportingHour"]; // 30
config.HoursbetweenNoMovementRep =   doc["HoursbetweenNoMovementRep"]; // 30

  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();

   // Dump config file

   
   if (debug_flag) {
       config.MinutesBetweenUploads = UPLOAD_INTERVALL_TEST;   // set for debug and test
       config.TimeOutLeavingSec = STATE_LEAVE_TEST;          // in seconds for test debug
       config.HoursbetweenNoMovementRep = 3;
       config.EveningReportingHour = 20;
   
       printFile(filename);
      }

  //     config.MorningReportingHour = 11;
  //   config.EveningReportingHour = 23;
  /*    
    if (debug_flag_push)            // do this whenever we test important functions
      config.HoursbetweenNoMovementRep = HOURSBETWEENNOMOV;
  */
      
     printConfig();           // do this every time
}
//--------------------------------------------------------------------




//-----------------------------------------------------------
// Prints the content of a file to the Serial
void printFile(const char *filename) {
  // Open file for reading
  File file = SD.open(filename);
  if (!file) {
    Serial.println(F("Failed to read file"));
    return;
  }

  // Extract each characters by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();

  // Close the file
  file.close();
}

//-----------------------------------------------------------
// Prints the config data of a file to the Serial
void printConfig() {    


 Serial.println ("Values: ");
  Serial.println (config.ThingSpeakChannelNo);
  Serial.println (config.ThingSpeakFieldNo);
  Serial.println (config.ThingSpeakWriteAPIKey);
  Serial.println (config.Title);
  Serial.println (config.PersonName);
  Serial.println (config.MinutesBetweenUploads);
  Serial.println (config.wlanssid_1);
  Serial.println (config.wlanpw_1);
  Serial.println (config.wlanssid_2);
  Serial.println (config.wlanpw_2);
   Serial.println (config.NTPPool);
  Serial.println (config.Timezone_Info);
  Serial.println (config.Email_1);
  Serial.println (config.Email_2);

 Serial.println (config.TimeOutLeavingSec); 
 Serial.println (config.MaxActivityCount);
 Serial.println (config.ScreenTimeOutSeconds); 
 Serial.println (config.PushoverUserkey); 
  Serial.println(config.PushoverToken); 

  Serial.println (config.PushoverDevices);
  Serial.println (config.HoursbetweenNoMovementRep);
  Serial.println (config.EveningReportingHour);
  Serial.println (config.MorningReportingHour);
  Serial.println ("--------------------");
  
  Serial.println ("credentials:");
  for (int i=0; i < 4; i++) {
   Serial.println(credentials[i]);
  }
  Serial.println ("--------------------");
}

// end of code ------------------------------
