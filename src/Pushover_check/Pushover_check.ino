
/*
 * https://github.com/ArduinoHannover/Pushover
 * 
 * mit meiner eigenen WiFi Library
 * 
 */
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

const char* ssid = "P-NETGEAR";
const char* password = "Hermannelsa-195]";

#define DEBUGLEVEL 1     // für Debug Output, für Produktion DEBUGLEVEL 0 setzen !
#include <DebugUtils.h>  // Library von Adreas Spiess

const char* apiToken = "ajiu7a1odbcx9xp6op2wenoctneeik";
const char* userToken = "uku7m4n8ru7rwf1wkmxvhbqniq4bxh";

//Pushover API endpoint
const char* pushoverApiEndpoint = "https://api.pushover.net/1/messages.json";

// Create a WiFiClientSecure object
WiFiClientSecure secure_client;

// Create an HTTPClient object
HTTPClient https;

//Pushover root certificate (valid from 11/10/2006 to 15/01/2038)
const char* PUSHOVER_ROOT_CA = "-----BEGIN CERTIFICATE-----\n"
                               "MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n"
                               "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
                               "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
                               "MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n"
                               "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
                               "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n"
                               "9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n"
                               "2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n"
                               "1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n"
                               "q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n"
                               "tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n"
                               "vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n"
                               "BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n"
                               "5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n"
                               "1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n"
                               "NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n"
                               "Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n"
                               "8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n"
                               "pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n"
                               "MrY=\n"
                               "-----END CERTIFICATE-----\n";

//------------------------------------------------
// displays at startup the Sketch running in the Arduino
void display_Running_Sketch(void) {
  String the_path = __FILE__;
  int slash_loc = the_path.lastIndexOf('/');
  String the_cpp_name = the_path.substring(slash_loc + 1);
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



//--------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(2000);
  display_Running_Sketch();
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");



  //Make HTTPS POST request to send notification
  if (WiFi.status() == WL_CONNECTED) {
  

    Serial.println("\nlets try to send a message...");

    // Create a JSON object with notification details
    // Check the API parameters: https://pushover.net/api
    StaticJsonDocument<512> notification;
    notification["token"] = apiToken;                   //required
    notification["user"] = userToken;                   //required
    notification["message"] = "Hello from pushover_8";  //required
    notification["title"] = "ESP32 Notification";       //optional
    notification["device"] = "petersiphone";            //optional
    notification["url"] = "";                           //optional
    notification["url_title"] = "";                     //optional
    notification["html"] = "";                          //optional
    notification["priority"] = "";                      //optional
    notification["sound"] = "bike";                     //optional
    notification["timestamp"] = "";                     //optional

    // Serialize the JSON object to a string
    String jsonStringNotification;
    serializeJson(notification, jsonStringNotification);
    Serial.println("Dies senden wir: ");
    Serial.println(jsonStringNotification);
  

    // Set the certificate for the WiFiClientSecure object
    secure_client.setCACert(PUSHOVER_ROOT_CA);
    // Specify the target URL
    https.begin(secure_client, pushoverApiEndpoint);

    // Add headers
    https.addHeader("Content-Type", "application/json");

    // Send the POST request with the JSON data
    int httpResponseCode = https.POST(jsonStringNotification);

    // Check the response
    if (httpResponseCode > 0) {
      Serial.printf("HTTP response code: %d\n", httpResponseCode);
      String response = https.getString();
      Serial.println("Response:");
      Serial.println(response);
    } else {
      Serial.printf("HTTP response code: %d\n", httpResponseCode);
      Serial.println(https.errorToString(httpResponseCode));
      // Close the connection
      https.end();
    }
    if (httpResponseCode == 200)
      Serial.printf("Pushover okok");
    else
      Serial.printf("Pushover FAILED");
  }
}

void loop() {
  delay(500);
}
