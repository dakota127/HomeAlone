/* -------------------------------------------------------------------
* Home Alone Application
* Based on a project presentd by Ralph Bacon
* This version used an ESP32 doing multitasking.
* by Peter B, from Switzerland
* Project website http://projects.descan.com/projekt7.html
* 

* JSON:
* arduinojson.org/v6/assistant 
* to compute the capacity.
*/

#define NUMBEROF_TRIES 2

const char*  push_server = "api.pushover.net";  // Server URL
// ThingSpeak settings
const char* thing_server = "api.thingspeak.com";
String tokenstring;
String userkeystring;
String devicestring;
String personstring;

// api.pushover.net root certificate authority
// SHA1 fingerprint is broken now!
const char* test_root_ca= \
     "-----BEGIN CERTIFICATE-----\n" \
     "MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
     "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
     "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
     "QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
     "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
     "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
     "9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
     "CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
     "nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
     "43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
     "T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
     "gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
     "BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
     "TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
     "DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
     "hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
     "06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
     "PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
     "YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
     "CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n" \
     "-----END CERTIFICATE-----\n";

//----------------------------------------------------------------

//--------------------------------------------------------------
int push_msg (String text, int prio){
  
// ------- report to pushover ----------------------------------


 //   return(0);

    DEBUGPRINTLN1 ("\t\tsend Pushover message");

         // set up parameter for this job
        wifi_todo = PUSH_MESG;
        wifi_order_struct.order = wifi_todo;
        wifi_order_struct.pushtext = text;
        wifi_order_struct.priority = prio;    
        ret = wifi_func();
        DEBUGPRINT2 ("\t\twifi_func returns: ");   DEBUGPRINTLN2 (ret);
        if (ret == 0) {           // reset count if ok 
         DEBUGPRINTLN1 ("\t\tmessage sent ok");
        }
        xSemaphoreTake(SemaOledSignal, portMAX_DELAY);    // signal oled task to switch display on
         oledsignal = 1;
        xSemaphoreGive(SemaOledSignal);        
        
        vTaskDelay(200 / portTICK_PERIOD_MS);
// ------- report to pushover ----------------------------------
     
}

/*
//--------------------------------------------------
//  Thingsspeak Stuff  
//-------------------------------------------------
int report_toCloud(int count) {
  
// NOte:
// For users of the free option, the message update interval limit remains limited at 15 seconds.
   DEBUGPRINT1 ("Cloud Update count: ");
   DEBUGPRINTLN1 (count);

  String th_string;
 // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
  // pieces of information in a channel.  Here, we write to field 1.
  if (!client2.connect(thing_server, 80)){
      
        Serial.println("Connection failed");

        client.stop();
        return (5);;     
    }
    
    else{
        
        // Create data string to send to ThingSpeak.
        String data = "count=" + String(count) ; //shows how to include additional field data in http post
      
        th_string =   config.ThingSpeakWriteAPIKey;
        // POST data to ThingSpeak.
        if (client2.connect(thing_server, 80)) {
          
            client2.println("POST /update HTTP/1.1");
            client2.println("Host: api.thingspeak.com");
            client2.println("Connection: close");
            client2.println("User-Agent: ESP32WiFi/1.1");
            client2.println("X-THINGSPEAKAPIKEY: "+ th_string);
            client2.println("Content-Type: application/x-www-form-urlencoded");
            client2.print("Content-Length: ");
            client2.print(data.length());
            client2.print("\n\n");
            client2.print(data);

          while (client2.connected()) {
      String line = client2.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("headers received");
        break;
      }
    }
    // if there are incoming bytes available
    // from the server, read them and print them:
    while (client2.available()) {
      char c = client2.read();
      Serial.write(c);
    }     
            }
    }
    client.stop();
    return (7);
  }

    // end report to cloud
*/

//--------------------------------------------------
//  Thingsspeak Stuff  
//-------------------------------------------------
int report_toCloud (int count) {
  
// NOte:
// For users of the free option, the message update interval limit remains limited at 15 seconds.
   DEBUGPRINT1 ("Cloud Update count: ");
   DEBUGPRINTLN1 (count);


 // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
  // pieces of information in a channel.  Here, we write to field 1.
  ThingSpeak.begin(client2);  // Initialize ThingSpeak

  int x = ThingSpeak.writeField (config.ThingSpeakChannelNo , 1, count, (char *)config.ThingSpeakWriteAPIKey);
  if(x == 200) {
    DEBUGPRINTLN1 ("Channel update successful.");
    return (0);
  }
  else {
    DEBUGPRINTLN0 ("Problem updating channel. HTTP error code " + String(x));
    return(x);
  }

 }    // end report to cloud

 
// -------------------------------------------------
// Pushover  stuff 
//----------------------------------------------------

int report_toPushover (String messageText, int prio) {

  DEBUGPRINT1 ("report_toPushover: "); DEBUGPRINT1 (messageText);  DEBUGPRINT1 ("  prio: "); DEBUGPRINTLN1 (prio);
  DEBUGPRINT1 ("devices: "); DEBUGPRINTLN1 (config.PushoverDevices);
  DEBUGPRINT1 ("token: "); DEBUGPRINT1 (config.PushoverToken); DEBUGPRINT1 ("  Key: "); DEBUGPRINTLN1 (config.PushoverUserkey);

  tokenstring = config.PushoverToken;
  userkeystring = config.PushoverUserkey;
  devicestring = config.PushoverDevices;
  personstring = config.PersonName;
  
  client.setCACert (test_root_ca);
  //client.setCertificate(test_client_key); // for client verification
  //client.setPrivateKey(test_client_cert);  // for client verification

  DEBUGPRINTLN2 ("\nStarting connection to pushover server...");
  
  if (!client.connect(push_server, 443))
    DEBUGPRINTLN2 ("Connection failed!");
  else {
    DEBUGPRINTLN2 ("Connected to pushover server");

//    Serial.println (messageText);

    vTaskDelay(300 / portTICK_PERIOD_MS); 
    
    int lendata = tokenstring.length() + userkeystring.length() + devicestring.length() + personstring.length() ;  
    
 //   Serial.print ("Länge daten: "); Serial.println (lendata);
 //   Serial.print ("Länge message: "); Serial.println (messageText.length());
    lendata = lendata + 36 + messageText.length();
 //   Serial.print ("Länge daten total: "); Serial.println (lendata);
    
    // Make a HTTP request:
    client.println ("POST /1/messages.json HTTP/1.1");
    client.println ("Host: api.pushover.net");
    client.println ("Content-Type: application/x-www-form-urlencoded");
    client.println ("Connection: close");
    client.print  ("Content-Length: ");
    client.print  (lendata);
    client.println("\r\n");
/*
    client.print ("token=");
    client.print (mytoken);
    client.print ("&user=");
    client.print(myuserkey);
    client.print ("&device=");
    client.print (mydevice);
    client.print ("&title=");
    client.print (mytitle);
    client.print ("&message=");
    client.print (message);
*/

//  //   36 literals plus 76 daten  = 112
    client.print("token=" + tokenstring + "&user=" + userkeystring + "&device=" + devicestring + "&title=" + personstring + "&message=" + messageText);

    
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("headers received from server");
        break;
      }
    }
    // if there are incoming bytes available
    // from the server, read them and print them:
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }

    client.stop();
  }
// else clause fertig

  DEBUGPRINT1 ("returcode Pushover: "); 
  DEBUGPRINTLN1 (ret);        //should return 1 on success aber gibt 0 bei ok
  if (ret == 1) ret = 0;      // convert this to 0
  
  return (ret);
}




// end of cde -----------------------------------



// end of code ------------------------------
