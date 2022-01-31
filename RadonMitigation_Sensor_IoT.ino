#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <Adafruit_MPRLS.h>
#include <RunningAverage.h>

// running average for pressure readings
RunningAverage pressureRA(20);
RunningAverage memRA(10);

// include your wifi settings
#include "my_wifi_settings.h"
// Network SSID
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// mprls pressure reader setup
// You dont *need* a reset and EOC pin for most uses, so we set to -1 and don't connect
#define RESET_PIN  -1  // set to any GPIO pin # to hard-reset on begin()
#define EOC_PIN    -1  // set to any GPIO pin to read end-of-conversion by pin
Adafruit_MPRLS mpr = Adafruit_MPRLS(RESET_PIN, EOC_PIN);

// IO Pins
const int led = LED_BUILTIN;
const int red = D5; // red
const int green = D7; // green
const int buzzard = D6;

// Pressure Inital values
float triggerPressure = 14.74; // normal
float pressure = 0;
int alarmCount = 0;
bool alarmOn = false;
String pressureStatus = "Unknown";
String mplrs = "Unknown";

// memory initial value
float mem = ESP.getFreeHeap();

// webserver settings
ESP8266WebServer server(80);
String text = "";
String postForms = "";

// Serving Status
void getStatus() {
  DynamicJsonDocument doc(512);
  doc["LV_PSI"] = pressure;
  doc["TR_PSI"] = triggerPressure;
  doc["RA_PSI"] = pressureRA.getFastAverage();
  doc["RA_MIN"] = pressureRA.getMin();
  doc["RA_MAX"] = pressureRA.getMax();
  doc["RA_STD"] = pressureRA.getStandardDeviation();
  doc["RA_MEM"] = memRA.getFastAverage();
  doc["RA_MEM_MIN"] = memRA.getMin();
  doc["RA_MEM_MAX"] = memRA.getMax();
  doc["AlarmOn"] = alarmOn;
  doc["AlarmCount"] = alarmCount;
  doc["MPLRS"] = mplrs;
  doc["Status"] = pressureStatus;
  
  Serial.print(F("Status..."));
  String buf;
  serializeJson(doc, buf);
  server.send(200, "application/json", buf);
  Serial.print(F("done."));
  Serial.println();
}

//Serving Settings
void getSettings() {
  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  //  StaticJsonDocument<512> doc;
  // You can use DynamicJsonDocument as well
  DynamicJsonDocument doc(512);
  doc["ip"] = WiFi.localIP().toString();
  doc["gw"] = WiFi.gatewayIP().toString();
  doc["nm"] = WiFi.subnetMask().toString();
  doc["signalStrengh"] = WiFi.RSSI();
  doc["chipId"] = ESP.getChipId();
  doc["flashChipId"] = ESP.getFlashChipId();
  doc["flashChipSize"] = ESP.getFlashChipSize();
  doc["flashChipRealSize"] = ESP.getFlashChipRealSize();
  doc["freeHeap"] = ESP.getFreeHeap();

  Serial.print(F("Settings..."));
  String buf;
  serializeJson(doc, buf);
  server.send(200, F("application/json"), buf);
  Serial.print(F("done."));
  Serial.println();
}

void handlePlain() {
  if (server.method() != HTTP_POST) {
    digitalWrite(led, 1);
    server.send(405, "text/plain", "Method Not Allowed");
    digitalWrite(led, 0);
  } else {
    digitalWrite(led, 1);
    triggerPressure = server.arg("newValue").toFloat();
    server.send(200, "text/plain", "true");
    digitalWrite(led, 0);
  }
}

// Define routing
void restServerRouting() {
  server.on(F("/status"), HTTP_GET, getStatus);
  server.on(F("/settings"), HTTP_GET, getSettings);
  server.on("/postplain/", handlePlain);
}

// Manage not found URL
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void connectWifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.hostname("RadionMitigationSensor");
  //WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected: ");
  Serial.print(WiFi.localIP());
  Serial.println();
}

void webServer() {
  // Set server routing
  restServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void beep() {
  digitalWrite(green, HIGH);
  for (int i = 0; i < 255; i = i + 2)
  {
    analogWrite(red, i);
    analogWrite(buzzard, i);
    delay(10);
  }
  for (int i = 255; i > 1; i = i - 2)
  {
    analogWrite(red, i);
    analogWrite(buzzard, i);
    delay(5);
  }
  for (int i = 1; i <= 10; i++)
  {
    analogWrite(red, 255);
    analogWrite(buzzard, 200);
    delay(100);
    analogWrite(red, 0);
    analogWrite(buzzard, 25);
    delay(100);
  }
}

void connectMPLRS() {
  if (! mpr.begin()) {
    mplrs = "Failed to communicate with MPRLS sensor, check wiring?";
    Serial.println(mplrs);
    while (1) {
      delay(10);
    }
  }
  mplrs = "Found MPRLS sensor";
  Serial.println(mplrs);
}

void readPressure() {
  float pressure_hPa = mpr.readPressure();
  pressure = (pressure_hPa / 68.947572932);
  // Serial.print("Pressure (PSI): "); Serial.print(String(pressure, 5));
  //Serial.println("");
  pressureRA.addValue(pressure);
}

void working(){
  pressureStatus = "working";
  Serial.println(pressureStatus);
  digitalWrite(red, HIGH); 
  digitalWrite(green, LOW);
  analogWrite(buzzard, 0);
}

void setup() {
  // Open Serial Port
  Serial.begin(9600);
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(buzzard, OUTPUT);
  pinMode(led, OUTPUT);
  digitalWrite(red, LOW);
  digitalWrite(green, LOW);
  delay(25);
  Serial.println("Radon Mitigation Monitoring System");
  analogWrite(buzzard, 0);
  connectWifi();
  webServer();
  connectMPLRS();
  pressureRA.clear(); // explicitly start clean
  working();
}

void loop() {
  server.handleClient();
  readPressure();
  if ( pressureRA.getFastAverage() >= triggerPressure ) {
    if ( !alarmOn ){
      alarmCount++;
      pressureStatus = "no vacuum detected";
      Serial.println(pressureStatus);
      digitalWrite(red, LOW); 
      digitalWrite(green, HIGH);
    }
    alarmOn = true;   
    beep(); 
  } else {
    if ( alarmOn ){
      working();
    }
    alarmOn = false;  
  }
  memRA.addValue(ESP.getFreeHeap());
}
