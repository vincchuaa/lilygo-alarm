
// => Hardware select
#define LILYGO_WATCH_2020_V3                 // To use T-Watch2020 V3, please uncomment this line
#define LILYGO_WATCH_LVGL 
// => Function select
#define LILYGO_WATCH_HAS_MOTOR       //Use Motor module 

#include <LilyGoWatch.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <HTTPClient.h>         //Remove Audio Lib error

#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include "AudioFileSourcePROGMEM.h"


// web
#define startupDBG      true
#define netDBG          true

#define dbg(b, s) if(b) Serial.print(s)
#define dln(b, s) if(b) Serial.println(s)

