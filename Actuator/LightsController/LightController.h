#include <Arduino.h>
#include <HTTPClient.h> // ESP32 library for making HTTP requests

#ifndef LIGHTCONTROLLER_H
#define LIGHTCONTROLLER_H

// Functions that set everything up
void basicsetup();
void lcsetup(); void lcloop();

// MAC address                                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
extern char MAC_ADDRESS[];
void getMAC(char *);

// LED utilities                                  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ledOn();
void ledOff();
void blink();

void setPendingWifi(bool b);
void setWifiConnection(bool b);

// Loop                                           ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
extern int loopIteration;         // a control iterator for slicing up the main loop 

// Wifi and HTTP server libraries                 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include <WiFi.h>
#include <WebServer.h>
#include "WiFiClientSecure.h"
#include <HTTPClient.h> // ESP32 library for making HTTP requests
#include <Update.h>     // OTA update library
// Globals for a wifi access point and webserver   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
extern String apSSID;           // SSID of the AP
extern WebServer webServer;

// Web page template
extern const char *boiler[];

typedef struct { int position; const char *replacement; } replacement_t;
void getHtml(String& html, const char *[], int, replacement_t [], int);
#define ALEN(a) ((int) (sizeof(a) / sizeof(a[0])))
#define GET_HTML(strout, boiler, repls) \
  getHtml(strout, boiler, ALEN(boiler), repls, ALEN(repls));
  
// Setting up web server, pages and routes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void startAP();
void printIPs();
void startWebServer();
void handleRoot();
void handleNotFound();
void handleWiFi();
void handleConnection();
void handleStatus();
void apListForm(String& f);

String ip2str(IPAddress address);

// Read requests from the watch                    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int readRequest(HTTPClient *, String);
// Controlling the LED watch based on the requests ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void controlLight();

// debugging infrastructure; setting different DBGs true triggers prints ////
#define dbg(b, s) if(b) Serial.print(s)
#define dln(b, s) if(b) Serial.println(s)
#define startupDBG      true
#define loopDBG         true
#define monitorDBG      true
#define netDBG          true
#define miscDBG         true
#define analogDBG       true
#define otaDBG          true

#endif