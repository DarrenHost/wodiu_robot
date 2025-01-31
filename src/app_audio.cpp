

#include "Arduino.h"
#include "WiFiMulti.h"
#include "Audio.h"
#include "SPI.h"
#include "SD.h"
#include "FS.h"

// Digital I/O used

#define I2S_DOUT      25
#define I2S_BCLK      27
#define I2S_LRC       26

Audio audio;
WiFiMulti wifiMulti;
String ssid =     "AP-Darren";
String password = "1q2w3e4r";

void audio_begin(){

   audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
   audio.setVolume(5); // 0...21
}
void audio_load(const char * url){
    audio.connecttohost(url); 
}

void audio_play(){
    audio.loop();
}

