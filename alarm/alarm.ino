 #pragma mark - Depend ESP8266Audio and ESP8266_Spiram libraries

#include "config.h"
#include "audio/pika.h"

#define EXTERNAL_DAC_PLAY   1

TTGOClass *watch;
TFT_eSPI *tft;
AudioGeneratorMP3 *mp3;
AudioFileSourcePROGMEM *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;

BMA *sensor;
bool irq = false;
char buf[128];
bool rtcIrq = false;

// Globals for a wifi access point and webserver //
String apSSID;                  // SSID of the AP
WebServer webServer(80);        // a simple web server

int lightRequest;

// Event handlder for light button 1
static void event_handler(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_CLICKED) {
    
        if ( lightRequest == 1) {
          lightRequest = 144;
          Serial.println(lightRequest);
        }
        else {
          lightRequest = 1;
          Serial.println(lightRequest);
        }
        
        Serial.printf("Clicked\n");
    } else if (event == LV_EVENT_VALUE_CHANGED) {
        Serial.printf("Toggled\n");
    }
}

// Event handlder for light button 1 
static void event_handler_2(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_CLICKED) {
    
        if ( lightRequest < 1000) {
          lightRequest += 1000;
          Serial.println(lightRequest);
        }
        else {
          lightRequest -= 1000;
          Serial.println(lightRequest);
        }
        
        Serial.printf("Clicked\n");
    } else if (event == LV_EVENT_VALUE_CHANGED) {
        Serial.printf("Toggled\n");
    }
}


void setup()
{
    Serial.begin(115200);          

    // Get TTGOClass instance
    watch = TTGOClass::getWatch();

    // Initialize the hardware, the BMA423 sensor has been initialized internally
    watch->begin();

    // Turn on the backlight and motor
    watch->motor_begin();
    watch->openBL();
    watch->lvgl_begin();
    
    //Receive objects for easy writing
    tft = watch->tft;
    sensor = watch->bma;
    
    startAP();            // fire up the AP...
    startWebServer(); 

    showTime();           // the RTC interface

    showLightsButton();   // displaying the light buttons

    // Accel parameter structure
    Acfg cfg;
    cfg.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
    cfg.range = BMA4_ACCEL_RANGE_2G;
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;
    
    sensor->accelConfig(cfg);
    sensor->enableAccel();
    
    // Setting up pinouts
    pinMode(BMA423_INT1, INPUT);
    pinMode(RTC_INT_PIN, INPUT_PULLUP);
    attachInterrupt(BMA423_INT1, [] {
        // Set interrupt to set irq value to 1
        irq = 1;
    }, RISING); //It must be a rising edge
    attachInterrupt(RTC_INT_PIN, [] {
        rtcIrq = 1;
    }, FALLING);
    
    // Enable BMA423 step count feature
    sensor->enableFeature(BMA423_STEP_CNTR, true);

    // Reset steps
    sensor->resetStepCounter();

    // Turn on step interrupt
    sensor->enableStepCountInterrupt();

    //Setting up initial alarm timer
    watch->rtc->disableAlarm();

    watch->rtc->setDateTime(2020, 8, 12, 15, 0, 55);

    watch->rtc->setAlarmByMinutes(1);

    watch->rtc->enableAlarm();

    //Setting up audiofile for alarm
    file = new AudioFileSourcePROGMEM(pika, sizeof(pika));
    id3 = new AudioFileSourceID3(file);
    
#if EXTERNAL_DAC_PLAY
    out = new AudioOutputI2S();
    out->SetPinout(TWATCH_DAC_IIS_BCK, TWATCH_DAC_IIS_WS, TWATCH_DAC_IIS_DOUT);
#else
    out = new AudioOutputI2S(0, 1);
#endif
 
    mp3 = new AudioGeneratorMP3();
    mp3->begin(id3, out);

    //!Turn on the audio power
    watch->enableLDO3();
    // Some display settings
    tft->setTextColor(random(0xFFFF));
    tft->drawString("BMA423 StepCount", 3, 50, 4);
    tft->setTextFont(4);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
}

void loop()
{   
    uint32_t step = sensor->getCounter();
    irq = 0;
    bool  rlst;
    do {
        // Read the BMA423 interrupt status,
        // need to wait for it to return to true before continuing
        rlst =  sensor->readInterrupt();
    } while (!rlst);
    //If alarm event is detected
    if (rtcIrq) {
        //Retrieve the step counter data
        rtcIrq = 0;
        
        detachInterrupt(RTC_INT_PIN);   //Remove RTC pin 
        watch->rtc->resetAlarm();       //Resets the alarm
        
        while( step < 10 ) {            //If step counter target is not reached
            step = sensor->getCounter();
            watch->motor->onec();
            if (mp3->isRunning()) {  
                if (!mp3->loop()) mp3->stop();
                Serial.println("yes");
            }
            if (!mp3->isRunning()){
              //Replaying audio
                mp3 = new AudioGeneratorMP3();
                file = new AudioFileSourcePROGMEM(pika, sizeof(pika));
                id3 = new AudioFileSourceID3(file);
                mp3->begin(id3, out);
            }
            Serial.println(mp3->isRunning());
            //Setting up alarm
            watch->tft->fillScreen(TFT_RED);
            watch->tft->setTextColor(TFT_WHITE, TFT_RED);
            watch->tft->drawString("Walk to Turn Off", 30, 98, 4);
            watch->tft->drawString("Alarm", 80, 148, 4);
        }
       watch->tft->fillScreen(TFT_WHITE);
    }
  showTime();
  showLightsButton();
  webServer.handleClient();
  lv_task_handler();
}

// Starting the WiFi provisioning
void startAP() {
  apSSID = String("coolwatch");
//  apSSID.concat(MAC_ADDRESS);

  if(! WiFi.mode(WIFI_AP_STA))
    dln(startupDBG, "failed to set Wifi mode");
  if(! WiFi.softAP(apSSID.c_str(), "dumbpassword"))
    dln(startupDBG, "failed to start soft AP");
  printIPs();
}

// Check IP addresses
void printIPs() {
  if(startupDBG) { // easier than the debug macros for multiple lines etc.
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

// Starting web server
void startWebServer() {
  // register callbacks to handle different paths
  webServer.on("/", handleRoot);
  webServer.on("/lights", controlLights);

  // 404s...
  webServer.onNotFound(handleNotFound);

  webServer.begin();
  dln(startupDBG, "HTTP server started");
}

// HTML page creation utilities //
String getPageTop() {
  return
    "<html><head><title>COM3506 IoT [ID: " + apSSID + "]</title>\n"
    "<meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
    "\n<style>body{background:#FFF; color: #000; "
    "font-family: sans-serif; font-size: 150%;}</style>\n"
    "</head><body>\n"
  ;
};
String getPageBody() {
  return "<h2>Welcome to Thing!</h2>\n";
}
String getPageFooter() {
  return "\n<p><a href='/'>Home</a>&nbsp;&nbsp;&nbsp;</p></body></html>\n";
}

void handleNotFound() { 
  dbg(netDBG, "URI Not Found: ");
  dln(netDBG, webServer.uri());
  webServer.send(200, "text/plain", "URI Not Found");
}

// Serving web page at root
void handleRoot() {
  dln(netDBG, "serving page notionally at /");
  String toSend = getPageTop();
  toSend += getPageBody();
  toSend += getPageFooter();
  webServer.send(200, "text/html", toSend);
}

// Serving web page at /lights; acting as a request to control the lights
void controlLights() {
  String toSend = String(lightRequest);
  webServer.send(200, "text/html", toSend);
}

// Interface of the light buttons
void showLightsButton() {
  lv_obj_t *label;

  lv_obj_t *lightBTN1 = lv_btn_create(lv_scr_act(), NULL);
  lv_obj_set_event_cb(lightBTN1, event_handler);
  lv_obj_align(lightBTN1, NULL, LV_ALIGN_CENTER, 45, -90);
  lv_btn_set_checkable(lightBTN1, true);
  lv_btn_toggle(lightBTN1);
  lv_btn_set_fit2(lightBTN1, LV_FIT_NONE, LV_FIT_TIGHT);

  label = lv_label_create(lightBTN1, NULL);
  lv_label_set_text(label, "Light 1");

  lv_obj_t *lightBTN2 = lv_btn_create(lv_scr_act(), NULL);
  lv_obj_set_event_cb(lightBTN2, event_handler_2);
  lv_obj_align(lightBTN2, NULL, LV_ALIGN_CENTER, 45, -40);
  lv_btn_set_checkable(lightBTN2, true);
  lv_btn_toggle(lightBTN2);
  lv_btn_set_fit2(lightBTN2, LV_FIT_NONE, LV_FIT_TIGHT);

  label = lv_label_create(lightBTN2, NULL);
  lv_label_set_text(label, "Light 2");
}

// Interface of the RTC
void showTime(){
  //Shows the initial setup time
    watch->tft->setTextColor(random(0xFFFF));
    watch->tft->drawString("Walk n'Wake", 50, 180, 4);
    watch->tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    snprintf(buf, sizeof(buf), "%s", watch->rtc->formatDateTime());
    watch->tft->drawString(buf, 10, 108, 7);
}
