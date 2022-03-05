#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebSocketsServer.h>

#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#include "camera_pins.h"

HTTPClient http;
WebSocketsServer websocket=WebSocketsServer(82);

int trigPin = 16;
int echoPin=0;  
#define pin1 4 //YELLOW
#define pin2 2 //BLUE
#define pin3 14 //ORANGE
#define pin4 15 //BLACK
int stepnumber=0;
void Onestep(bool dir){
  if(dir){
    switch(stepnumber){
      case 0:
        digitalWrite(pin1,HIGH);
        digitalWrite(pin2,LOW);
        digitalWrite(pin3,LOW);
        digitalWrite(pin4,LOW);
        break;
      case 1:
        digitalWrite(pin1,LOW);
        digitalWrite(pin2,HIGH);
        digitalWrite(pin3,LOW);
        digitalWrite(pin4,LOW);
        break;
      case 2:
        digitalWrite(pin1,LOW);
        digitalWrite(pin2,LOW);
        digitalWrite(pin3,HIGH);
        digitalWrite(pin4,LOW);
        break;
       case 3:
        digitalWrite(pin1,LOW);
        digitalWrite(pin2,LOW);
        digitalWrite(pin3,LOW);
        digitalWrite(pin4,HIGH);
        break;
    }
  }else{
     switch(stepnumber){
      case 0:
        digitalWrite(pin1,LOW);
        digitalWrite(pin2,LOW);
        digitalWrite(pin3,LOW);
        digitalWrite(pin4,HIGH);
        break;
      case 1:
        digitalWrite(pin1,LOW);
        digitalWrite(pin2,LOW);
        digitalWrite(pin3,HIGH);
        digitalWrite(pin4,LOW);
        break;
      case 2:
        digitalWrite(pin1,LOW);
        digitalWrite(pin2,HIGH);
        digitalWrite(pin3,LOW);
        digitalWrite(pin4,LOW);
        break;
       case 3:
        digitalWrite(pin1,HIGH);
        digitalWrite(pin2,LOW);
        digitalWrite(pin3,LOW);
        digitalWrite(pin4,LOW);
        break;
    }
  }
  stepnumber++;
  if(stepnumber>3){
    stepnumber=0;
  }
}  

void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length){
  switch(type){
    case WStype_DISCONNECTED:
      Serial.println("client disconnected");
      break;
    case WStype_CONNECTED:
      Serial.println("client connected");
      break;
    case WStype_TEXT:
      String text="";
      String message;
      for(int i=0;i,length;i++){
        text=text+(char)payload[i];
      }
      //Serial.println(text);
       http.begin("http://169.254.223.163/read.php?ID=1");
       //http.begin("http://172.20.10.2/read.php?ID=1");
       int httpCode = http.GET();
       if(httpCode > 0){
        message = http.getString();
        Serial.println(message);
       }else{
        Serial.println("error in http request");
      }
      http.end();
      if(text == message){
         for(int i=0;i<=12;i++){
            Onestep(true);
        }
        delay(500);
        for(int i=0;i<=12;i++){
        Onestep(false);
        }
        delay(500);
       }
      break;
  }
}

const char* ssid = "*********";
const char* password = "*********";

void startCameraServer();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  
  pinMode(trigPin,OUTPUT);
  pinMode(echoPin,INPUT);


  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
  websocket.begin();
  websocket.onEvent(webSocketEvent);
}

void loop() {
  int interval = 1000;
  unsigned long previousMillis=0;
  float distance, duration;
  websocket.loop();
  digitalWrite(trigPin,LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin,HIGH);
  delayMicroseconds(2);
  digitalWrite(trigPin,LOW);

  duration=pulseIn(echoPin,HIGH);
  distance=(duration/2)*0.0343;

  if (distance>=400 || distance <=2){
    Serial.println("distance out of range");
  }else if (distance>20 && distance <=30){
     http.begin("http://169.254.223.163/update.php?ID=1&available=\"NO\"");
    //http.begin("http://172.20.10.2/update.php?ID=1&available=\"free\"");
    int httpCode = http.GET();
    if(httpCode > 0){
      String message = http.getString();
      Serial.println(message);
    }else{
      Serial.println("error in http request");
    }
    http.end();
    //socket code here and ocr
    unsigned long now =millis();
   if(now-previousMillis>interval){
     String q="qwerty";
     int str_len=q.length()+1;
     char CharArray[str_len];
     q.toCharArray(CharArray,str_len);
     websocket.broadcastTXT(CharArray);
     previousMillis=now;
   }
  }else if (distance>2|| distance <=20){
     http.begin("http://169.254.223.163/update.php?ID=1&available=\"YES\"");
    //http.begin("http://172.20.10.2/update.php?ID=1&available=\"taken\"");
    int httpCode = http.GET();
    if(httpCode > 0){
      String message = http.getString();
      Serial.println(message);
    }else{
      Serial.println("error in http request");
    }
    http.end();
  }

  delay(10000);
}
