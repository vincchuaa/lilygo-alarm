
#include <Wire.h>
#include <esp_log.h>
#include "LightController.h"
#include <WiFiClientSecure.h>

char MAC_ADDRESS[13];           // MAC ADDRESS
String apSSID;                  // SSID of the AP
WebServer webServer(80);    

// Initialisation               ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void basicsetup() {
  Serial.begin(115200);         // initialise the serial line
  getMAC(MAC_ADDRESS);          // store the MAC address as a chip identifier
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(12, OUTPUT);
}

// Get the MAC address of the actuator/controller ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void getMAC(char *buf) {                   // the MAC is 6 bytes, so needs careful conversion...
  uint64_t mac = ESP.getEfuseMac();        // ...to string (high 2, low 4):
  char rev[13];
  sprintf(rev, "%04X%08X", (uint16_t) (mac >> 32), (uint32_t) mac);

  // the byte order in the ESP has to be reversed relative to normal Arduino
  for(int i=0, j=11; i<=10; i+=2, j-=2) {
    buf[i] = rev[j - 1];
    buf[i + 1] = rev[j];
  }
  buf[12] = '\0';
}

// Start access point. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void startAP() {
  apSSID = String("Thing-");
  apSSID.concat(MAC_ADDRESS);

  if(! WiFi.mode(WIFI_AP_STA))
    dln(startupDBG, "failed to set Wifi mode");
  if(! WiFi.softAP(apSSID.c_str(), "dumbpassword"))
    dln(startupDBG, "failed to start soft AP");

  delay(100);

  // Configure SoftAP address, to avoid clashes.
  if(! WiFi.softAPConfig(IPAddress(192, 168, 1, 2), IPAddress(192, 168, 1, 2), IPAddress(255, 255, 255, 0)))
    Serial.println("STA failed to configure!");
  
  printIPs();
}

// Print the IP address of the actuator/controller            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void printIPs() {
  if(startupDBG) {                                // easier than the debug macros for multiple lines etc.
    Serial.print("AP SSID: ");
    Serial.print(apSSID);
    Serial.print("; IP address(es): local=");
    Serial.print(WiFi.localIP());
    Serial.print("; AP=");
    Serial.println(WiFi.softAPIP());
  }
  if(netDBG)
    WiFi.printDiag(Serial);
}

// Start the Web Server of the actuator/controller             ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void startWebServer() {
  // register callbacks to handle different paths 
  webServer.on("/", handleRoot);
  // 404s...
  webServer.onNotFound(handleNotFound);

  webServer.on("/wifi", handleWiFi);                    // UI for displaying the available connection
  webServer.on("/wifi-connecting", handleConnection);   // UI for pending connection
  webServer.on("/wifi-status", handleStatus);           // UI for displaying the connection status

  webServer.begin();
  dln(startupDBG, "HTTP server started");
}

// Web Template                       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
const char *templatePage[] = {
  "<html><head><title>",                                                //  0
  "default title",                                                      //  1
  "</title>\n",                                                         //  2
  "<meta charset='utf-8'>",                                             //  3
  "<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n"
  "<style>body{background:#FFF; color: #000; font-family: sans-serif;", //  4
  "font-size: 150%;}</style>\n",                                        //  5
  "</head><body>\n",                                                    //  6
  "<h2>Light Status: </h2>\n",                                          //  7
  "<!-- page payload goes here... -->\n",                               //  8
  "<!-- ...and/or here... -->\n",                                       //  9
  "\n<p><a href='/'>Back to Home</a>&nbsp;&nbsp;&nbsp;</p>\n",          // 10
  "</body></html>\n\n",                                                 // 11
};

// UI for the root page, which shows the status of the light    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void handleRoot() {
  dln(netDBG, "serving page notionally at /");
  String line9 = "<p>Choose a <a href=\"wifi\">Wifi access point</a>.</p>";

  if (WiFi.status() == WL_CONNECTED){
    line9 = "<p> <a href=\"update-firmware\">Update ESP32 firmware</a></p>\n"
            "<p> <a href=\"wifi\">Change connection point</a>.</p>";
  }
  if (WiFi.status() == WL_CONNECT_FAILED){
    line9 = "<p>Connection Failed... :( </p>\n"
            "<p> <a href=\"wifi\">Connect Manually.</a>.</p>";
  }
  if (WiFi.status() == WL_DISCONNECTED){
    line9 = "<p>Disconnected to the watch, please connect again.</p>\n"
            "<p> <a href=\"wifi\">Connect Manually.</a>.</p>";
    setWifiConnection(false);
    setPendingWifi(true);
  }
  replacement_t repls[] = {             // the elements to replace in the boilerplate
    {  1, apSSID.c_str() },
    {  8, "" },
    {  9, line9.c_str() },
    { 10, "<p>Check <a href='/wifi-status'>Wifi status</a>.</p>" },
  };
  String htmlPage = "";                 // a String to hold the resultant page
  GET_HTML(htmlPage, templatePage, repls); 
  webServer.send(200, "text/html", htmlPage);
}

// Webserver handler callbacks                                   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void handleNotFound() {
  dbg(netDBG, "URI Not Found: ");
  dln(netDBG, webServer.uri());
  webServer.send(200, "text/plain", "URI Not Found");
  dln(startupDBG, "HTTP server started");
}

// UI for displaying the list of available connections via WiFi  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void handleWiFi() {
  dln(netDBG, "serving page at /wifi");
  setPendingWifi(true);
  String form = "";                              // a form for choosing an access point and entering key
  apListForm(form);
  replacement_t repls[] = {                      // the elements to replace in the boilerplate
    { 1, apSSID.c_str() },
    { 7, "<h2>Network configuration</h2>\n" },
    { 8, "" },
    { 9, form.c_str() },
  };
  String htmlPage = "";                          // a String to hold the resultant page
  GET_HTML(htmlPage, templatePage, repls);       // combining the web elements

  webServer.send(200, "text/html", htmlPage);
}

// Utility to create a form for choosing access points.          ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void apListForm(String& f) { 
  const char *checked = " checked";
  int n = WiFi.scanNetworks();
  dbg(netDBG, "scan done: ");

  if(n == 0) {
    dln(netDBG, "no networks found");
    f += "No wifi access points found :-( ";
    f += "<a href='/'>Back</a><br/><a href='/wifi'>Try again?</a></p>\n";
  } else {
    dbg(netDBG, n); dln(netDBG, " networks found");
    f += "<p>Please connect to the Wifi to update firmware.</p>\n"
         "<p>Available Wifi Access Points:</p>\n"
         "<p><form method='POST' action='wifi-connecting'> ";
    for(int i = 0; i < n; ++i) {
      f.concat("<input type='radio' name='ssid' value='");
      f.concat(WiFi.SSID(i));
      f.concat("'");
      f.concat(checked);
      f.concat(">");
      f.concat(WiFi.SSID(i));
      f.concat(" (");
      f.concat(WiFi.RSSI(i));
      f.concat(" dBm)");
      f.concat("<br/>\n");
      checked = "";
    }
    f += "<br/>Wifi Password: <input type='textarea' name='key'> ";
    f += "<input type='submit' value='Submit'></form></p><br/>";
  }
}

//  UI for pending connection                   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void handleConnection() {
  dln(netDBG, "serving page at /wifi-connecting");

  String title = "<h2>Joining wifi network...</h2>";
  String message = "<p>Check <a href='/wifi-status'>wifi status</a>.</p>";
  String ssid = "";
  String key = "";

  for(uint8_t i = 0; i < webServer.args(); i++ ) {
    if(webServer.argName(i) == "ssid")
      ssid = webServer.arg(i);
    else if(webServer.argName(i) == "key")
      key = webServer.arg(i);
  }

  setPendingWifi(false);
  if(ssid == "") {
    message = "<h2>Ooops, no SSID...?</h2>\n<p>Looks like a bug :-(</p>";
    setWifiConnection(false);
  } else {
    char ssidchars[ssid.length()+1];
    char keychars[key.length()+1];
    ssid.toCharArray(ssidchars, ssid.length()+1);
    key.toCharArray(keychars, key.length()+1);
    WiFi.begin(ssidchars, keychars);
    setWifiConnection(true);
  }

  replacement_t repls[] = { // the elements to replace in the template
    { 1, apSSID.c_str() },
    { 7, title.c_str() },
    { 8, "" },
    { 9, message.c_str() },
  };
  String htmlPage = "";     // a String to hold the resultant page
  GET_HTML(htmlPage, templatePage, repls);

  webServer.send(200, "text/html", htmlPage);
}

// Interface for checking connectivity.       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void handleStatus() {         
  dln(netDBG, "serving page at /wifi-status");

  String s = "";
  s += "<ul>\n";
  s += "\n<li>SSID: ";
  s += WiFi.SSID();
  s += "</li>";
  s += "\n<li>Status: ";
  switch(WiFi.status()) {
    case WL_IDLE_STATUS:
      s += "WL_IDLE_STATUS</li>"; break;
    case WL_NO_SSID_AVAIL:
      s += "WL_NO_SSID_AVAIL</li>"; break;
    case WL_SCAN_COMPLETED:
      s += "WL_SCAN_COMPLETED</li>"; break;
    case WL_CONNECTED:
      s += "WL_CONNECTED</li>"; break;
    case WL_CONNECT_FAILED:
      s += "WL_CONNECT_FAILED</li>"; break;
    case WL_CONNECTION_LOST:
      s += "WL_CONNECTION_LOST</li>"; break;
    case WL_DISCONNECTED:
      s += "WL_DISCONNECTED</li>"; break;
    default:
      s += "unknown</li>";
  }

  s += "\n<li>Local IP: ";     s += ip2str(WiFi.localIP());
  s += "</li>\n";
  s += "\n<li>Soft AP IP: ";   s += ip2str(WiFi.softAPIP());
  s += "</li>\n";
  s += "\n<li>AP SSID name: "; s += apSSID;
  s += "</li>\n";

  s += "</ul></p>";

  replacement_t repls[] = {         // the elements to replace in the boilerplate
    { 1, apSSID.c_str() },
    { 7, "<h2>Status</h2>\n" },
    { 8, "" },
    { 9, s.c_str() },
  };
  String htmlPage = "";             // a String to hold the resultant page
  GET_HTML(htmlPage, templatePage, repls);

  webServer.send(200, "text/html", htmlPage);
}

// Turn array of strings & set of replacements into a String.   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void getHtml( 
  String& html, const char *boiler[], int boilerLen,
  replacement_t repls[], int replsLen
){
  for(int i = 0, j = 0; i < boilerLen; i++) {
    if(j < replsLen && repls[j].position == i)
      html.concat(repls[j++].replacement);
    else
      html.concat(boiler[i]);
  }
    
}

String ip2str(IPAddress address) { // utility for printing IP addresses
  return
    String(address[0]) + "." + String(address[1]) + "." +
    String(address[2]) + "." + String(address[3]);
}

