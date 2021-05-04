/*
    ESP-CAM
*/

#include <Arduino.h>
#include <WiFi.h>
#include "soc/soc.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <EEPROM.h>

//Configuracoes Temperatura
#define ONE_WIRE_BUS 12 //variavel do pino 4 que esta plugado o Sensor de Temperatura
#define PINO_RELE 13

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

 const char* ssid = "AGREGADO";
 const char* password = "5px0p3r@c@0";

//const char* ssid = "ReiNaN";
//const char* password = "91dc116669";

//const char* ssid = "Projetos";
//const char* password = "91dc116669";


OneWire oneWire(ONE_WIRE_BUS); //Instacia o Objeto oneWire e Seta o pino do Sensor para iniciar as leituras
DallasTemperature temperature_sensor(&oneWire); //Repassa as referencias do oneWire para o Sensor Dallas (DS18B20)
WiFiClient client;


String serverName = "slumpmix-mockup.herokuapp.com";   // REPLACE WITH YOUR Raspberry Pi IP ADDRESS
String serverPath = "/webcam";     // The default serverPath should be upload.php

const int serverPort = 80;
int stateRelay;
const int timerInterval = 30000;    // time between each HTTP POST image
const int restartTimer = 900000; 
unsigned long restartMillis = 0;
unsigned long previousMillis = 0;   // last time image was sent




void setup() {
  EEPROM.begin(1);

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  Serial.begin(115200);

  pinMode(PINO_RELE, OUTPUT);
  stateRelay = EEPROM.read(0);
  Serial.println("Estado Rele:");
  Serial.println(stateRelay);
  digitalWrite(PINO_RELE , stateRelay);


  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);  

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());

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

  // init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  sendPhoto(); 

}

void loop() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= timerInterval) {

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Reconnecting to WiFi...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);  

      while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
      }
      
    }

    sendPhoto();

    previousMillis = currentMillis;
  }

}

String sendPhoto() {
  //Sensor de Temperatura
  temperature_sensor.requestTemperatures();
  float temp = temperature_sensor.getTempCByIndex(0);
  Serial.print("Temperatura : ");
  Serial.println(temp); 
  
  String getAll;
  String getBody;

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }
  
  Serial.println("Connecting to server: " + serverName);

  if (client.connect(serverName.c_str(), serverPort)) {
    Serial.println("Connection successful!");    
    String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"img\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--RandomNerdTutorials--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;
  
    client.println("POST " + serverPath + "?t=" + temp + "&r=" + stateRelay + " HTTP/1.1");
    client.println("Host: " + serverName);
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
    client.println();
    client.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0; n<fbLen; n=n+1024) {
      if (n+1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        client.write(fbBuf, remainder);
      }
    }   
    client.print(tail);
    
    esp_camera_fb_return(fb);
    
    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + timoutTimer) > millis()) {
      Serial.print(".");
      delay(100);      
      while (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (getAll.length()==0) { state=true; }
          getAll = "";
        }
        else if (c != '\r') { getAll += String(c); }
        if (state==true) { getBody += String(c); }
        startTimer = millis();
      }
      if (getBody.length()>0) { break; }
    }

    Serial.println();
    client.stop();
    Serial.println("bunda");

    if(String(getBody.charAt(1)) == "d"){
      digitalWrite(PINO_RELE , LOW);
      stateRelay = 0;
      EEPROM.write(0,stateRelay);
      EEPROM.commit();
      Serial.println(EEPROM.read(0));
      Serial.println("desligou rele");
    }

    else if(String(getBody.charAt(1)) == "l"){
      digitalWrite(PINO_RELE , HIGH);
      stateRelay = 1;
      EEPROM.write(0, stateRelay);
      EEPROM.commit();
      Serial.println(EEPROM.read(0));
      Serial.println("Ligou rele");
    }

  }
  else {
    getBody = "Connection to " + serverName +  " failed.";
    Serial.println(getBody);
  }

  return getBody;
}
