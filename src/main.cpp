#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Arduino_JSON.h>

// pines
const byte potentiometer = A0, //ADC
           led_pwm = 16;       //D0

// put function declarations here:
String getSensorReadings();
void initFS();
void initWiFi();
void notifyClients(String sensorReadings);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void initWebSocket();

// variables
const char* ssid = "ControlDeNivel";
const char* password = "patronato1914";

IPAddress ip(192,168,1,200);     
IPAddress gateway(192,168,1,1);   
IPAddress subnet(255,255,255,0);  

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
JSONVar readings;

unsigned long lastTime=0;

void setup() {
  // put your setup code here, to run once:
  pinMode(potentiometer,INPUT);
  pinMode(led_pwm,OUTPUT);
  analogWriteRange(1023);
  analogWriteFreq(1000);
  analogWrite(led_pwm,0);
  Serial.begin(115200);
  initWiFi();
  initFS();
  initWebSocket();
  server.on("/",HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.serveStatic("/",LittleFS,"/");
  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  if((millis()-lastTime)>1000){
    String sensorReading = getSensorReadings();
    Serial.println(sensorReading);
    notifyClients(sensorReading);
    lastTime=millis();
  }
  ws.cleanupClients();
}

// put function definitions here:
String getSensorReadings(){
  readings["potenciometro"] = String(analogRead(potentiometer));
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void initFS(){
  if(!LittleFS.begin()){
    Serial.println("Error montando LittleFS.");
  }
  else{
    Serial.println("LittleFS se montó correctamente.");
  }
}

void initWiFi(){
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(ip, gateway, subnet);
  Serial.print("Iniciando AP:\t");
  Serial.println(ssid);
  Serial.print("IP address:\t");
  Serial.println(WiFi.softAPIP());
}

void notifyClients(String sensorReadings){
  ws.textAll(sensorReadings);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len){
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT){
    String sensorReadings = getSensorReadings();
    Serial.print(sensorReadings);
    notifyClients(sensorReadings);
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket(){
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}