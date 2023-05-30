// la1.ino
#include <Arduino.h>
#include <Wire.h>
#include <esp_log.h>
#include "LightController.h"

// Loop attributes
int loopIteration = 0;
// LED attributes
bool settingUp = false;
bool pendingWifi = true;
bool connectedToWifi = false;
bool updating = false;
bool updateFailed = false;

// Watch softAP IP address
#define WATCH_SERVER_IP_ADDR "192.168.4.1" 

// Setup                      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void lcsetup() {
  settingUp = true;

  // Basic setups including the web pages
  basicsetup();
  pinMode(LED_BUILTIN, OUTPUT);   // set up GPIO pin for built-in LED
  Serial.println("\n Setting up Lights Controller...");

  startAP();                      // fire up the access point
  startWebServer();
  WiFi.begin("", "");             // Connecting to the watch
  
  // Red LED
  blink(10, 500, 6); 
  delay(5000);                    // giving time for browser to load up
  settingUp = false;
}

// Loop                       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void lcloop() {
  if(loopIteration % 5000000 == 0)   // debug print each 5000000
    dln(netDBG, "calling webServer.handleClient()...");
  
  int sliceSize = 500000;
  loopIteration++;
  if(loopIteration % sliceSize == 0) // a slice every sliceSize iterations
    dln(otaDBG, "OTA loop");
  
  // LED indications
  if(settingUp){
    blink(1, 300, 6);
  }
  if(pendingWifi){
    blink(1,700, 9);
  }
  if(WiFi.status() == WL_CONNECTED) {
    ledOn(9);
    controlLight();                  // Read request from the watch
  }
  if(WiFi.status() == WL_DISCONNECTED){
    setPendingWifi(true);            // make the yellow light blink to indicate pending connection
    ledOff(9);                       // off the yellow light
  }
  
  webServer.handleClient();
}

// Set sepecifc LED to blink ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void blink(int times, int pause, int pin) {
  ledOff(pin);
  for(int i=0; i<times; i++) {
    ledOn(pin); delay(pause); ledOff(pin); delay(pause);
  }
}
// Turning specic LED on    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ledOn(int pin) {
  digitalWrite(pin, HIGH);
}
// Turning specic LED off   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ledOff(int pin) {
  digitalWrite(pin, LOW);
}
// Set boolean for waiting wifi connection      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void setPendingWifi(bool b) {
  pendingWifi = b;
}
// Set wifi connection status                   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void setWifiConnection(bool b) {
  connectedToWifi = b;
}
// Get firmware update status                   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool getUpdateStatus() {
  return updateFailed;
}

// Read the light request from the watch        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int readRequest(HTTPClient *http, String fileName) {
  String url =
    String("http://") + WATCH_SERVER_IP_ADDR + "/" + fileName;
    Serial.printf("getting %s\n", url.c_str());
  // make GET request and return the response code
  http->begin(url);
  return http->GET();
}

// Controll the LED lights based on the request  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void controlLight(){
  // materials for doing an HTTPS GET on github from the BinFiles/ dir
  HTTPClient http;
  int respCode;
  String payload;

  // read the request sent from the watch
  respCode = readRequest(&http, "lights");
  Serial.printf("Response CODE is %d\n", respCode);
  if(respCode > 0){                  // check response code (-ve on failure)
    payload = http.getString();
    Serial.printf("Payload length is %d\n", payload.length());
  }  
  else
    Serial.printf("couldn't get version! rtn code: %d\n", respCode);
    updateFailed = true;
  http.end();                        // free resources
  
  // Controlling the lights
  if(payload.length() == 1){
    Serial.printf("Turning on lights ...");
    ledOn(12);                      // turn on light no 1
  }
  if(payload.length() == 3){
    Serial.printf("Turning off lights ...");
    ledOff(12);                     // turn on light no 1
  }

  if(payload.length() >= 4){
    ledOn(5);                       // turn on light no 2
  }
  if(payload.length() < 4){
    ledOff(5);                      // turn on light no 2
  }
    
}