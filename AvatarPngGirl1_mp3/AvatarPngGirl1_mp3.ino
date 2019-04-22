#include <M5Stack.h>
#include <WiFi.h>
#include <M5StackUpdater.h>
#include "src/avatar.h"
#include "src/eye.h"
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

AudioGeneratorMP3 *mp3;
AudioFileSourceSD *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;

Avatar *avatar;
Eye *eye;
TaskHandle_t taskHandle1;
TaskHandle_t taskHandle2;


String PlayFileName = "";
String DisplayFileName = "";
int selectNum;
String fileName[255];
int fileNum = 0;

void drawLoop(void *args)
{
  for(;;)
  {
    if (mp3->isRunning()) {
      int level = out->getLevel();
//    Serial.print("level:");
//    Serial.println(abs(level));
      level = abs(level);
      if(level > 20000) level = 20000;
      float open = (float)level/20000.0;
      avatar->setMouthOpen(open);
    }
    else
    {
      avatar->setMouthOpen(0.0);
    }
    avatar->draw();
    delay(100);
  }
}

void blink(void *args)
{
  for(;;)
  {
    eye->blink();
  }
}

void startAvatar()
{
  avatar = new Avatar();
  eye = new Eye(80, 75, 160, 44);
  xTaskCreatePinnedToCore(
                    drawLoop,     /* Function to implement the task */
                    "drawLoop",   /* Name of the task */
                    2048,      /* Stack size in words */
                    NULL,      /* Task input parameter */
                    1,         /* Priority of the task */
                    &taskHandle1,      /* Task handle. */
                    1);        /* Core where the task should run */
  xTaskCreatePinnedToCore(
                    blink,     /* Function to implement the task */
                    "blink",   /* Name of the task */
                    2048,      /* Stack size in words */
                    NULL,      /* Task input parameter */
                    2,         /* Priority of the task */
                    &taskHandle2,      /* Task handle. */
                    1);        /* Core where the task should run */  
}

void setup()
{
  M5.begin();
  Wire.begin();
  if(digitalRead(BUTTON_A_PIN) == 0) {
    Serial.println("Will Load menu binary");
    updateFromFS(SD);
    ESP.restart();
  }
  M5.Lcd.setBrightness(30);
  M5.Lcd.clear();
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextFont(2);
  WiFi.mode(WIFI_OFF); 

  out = new AudioOutputI2S(0, 1); // Output to builtInDAC
  out->SetOutputModeMono(true);
  float volume = 20.0;
  out->SetGain(volume/100.0);
  mp3 = new AudioGeneratorMP3();
  
  startAvatar(); // start drawing
}

void stopPlaying()
{
  if (mp3) {
    mp3->stop();
  }
  if (id3) {
    id3->close();
    delete id3;
    id3 = NULL;
  }
  if (file) {
    file->close();
    delete file;
    file = NULL;
  }
}

void loop()
{
  M5.update();
  if (M5.BtnB.wasPressed())
  {
    if (mp3->isRunning()) {
      stopPlaying();
    }
//    M5.Lcd.printf("Sample MP3 playback begins...\n");
    Serial.printf("Sample MP3 playback begins...\n");
//    file = new AudioFileSourceSD("/test.mp3");
    vTaskSuspend(taskHandle1);
    vTaskSuspend(taskHandle2);
    delay(500);
    scanDataFolder();
    avatar->init();
    delay(100);
    vTaskResume(taskHandle1);
    vTaskResume(taskHandle2);    
    file = new AudioFileSourceSD(PlayFileName.c_str());
    id3 = new AudioFileSourceID3(file);
    mp3->begin(id3, out);    
  }

  if (M5.BtnC.wasPressed())
  {
    Serial.printf("MP3 done\n");
    stopPlaying();
  }
  if (mp3->isRunning()) {
    if (!mp3->loop()) {
      stopPlaying();
    }
  } else {
    delay(100);
  }
  
}

void scanDataFolder() {
   File file;
   int i = 0;
   boolean exitMenu = false;

  Serial.println("scanDataFolder");

//  if(!SPIFFS.begin(true)){
//    M5.Lcd.println("SPIFFS Mount Failed");
//    Serial.println("SPIFFS Mount Failed");
//  } else {
    M5.Lcd.println("Scan data folder");
    Serial.println("Scan data folder");
    File root = SD.open("/mp3");
    if(!root){
      M5.Lcd.println("Failed to open directory");
    } else {
      M5.Lcd.println("Scan: " + (String)root.name());
      if(!root.isDirectory()){
        M5.Lcd.println("Not a directory");
      } else {
        while(i < 256) {
          file = root.openNextFile();
          if(!file){
//            M5.Lcd.println("Total " + (String)i + " files");
            fileNum = i;
            i = 256;
          } else {
            fileName[i] = file.name();
//            M5.Lcd.println(fileName[i]);
          }
          i++;
        }
      }
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.println("*** File List ***");

      selectNum = 0;
      listFolder();

      while (!exitMenu){
        if(digitalRead(BUTTON_A_PIN) == 0) {
          if( selectNum > 0){
            selectNum--;
            listFolder();
          }
        }
        if(digitalRead(BUTTON_C_PIN) == 0) {
          if( selectNum < fileNum -1){
            selectNum++; 
            listFolder();
          }
        }
        if(digitalRead(BUTTON_B_PIN) == 0) {
          exitMenu = true;
        }
        delay(100);
      }

      PlayFileName = fileName[selectNum];
    }
//  }
}

void listFolder() {
  M5.Lcd.setCursor(0, 30);
  for(int i = 0; i < fileNum;  i++){
    if(i == selectNum){
      M5.Lcd.println("> " + fileName[i]);
    } else {
      M5.Lcd.println("  " + fileName[i]);
    }
  }
}


