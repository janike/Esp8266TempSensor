#include <ArduinoJson.h>
#include "DHT.h"
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

#include "html_root.h"
#include "html_config.h"

#define DEBUG
#ifdef DEBUG
 #define DEBUG_PRINT(x)  Serial.print (x)
 #define DEBUG_PRINTLN(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTLN(x)
#endif


#define AP_SSID "Esp8266"
#define AP_PASS "12345678"

//analog read constant to calculate input volgate - depends on voltage divider ratio
//#define ANALOG_READ_CONSTANT 0.00698 // 1st version
//#define ANALOG_READ_CONSTANT 0.006303 // 2nd version
#define ANALOG_READ_CONSTANT 0.006303 // esp temp board 1.0

#define DHTPIN 13     //D3 on 1st version, D1 on 2nd version, 13 on esp temp board 1.0
#define DHTTYPE DHT22 //DHT11 / DTH22
#define BUTTON_PIN D2
#define LED_PIN D4

#define EEPROM_OFFSET 60
#define SENSOR_RETRIES 0
#define SLEEPTIME 300       // seconds
#define ERROR_SLEEPTIME 300 // seconds
#define BLINK_DELAY 100     // miliseconds

struct s_settings {
  char c;
  char ssid[20];
  char passwd[15];
  bool staticIp;
  char mqttIp[15];
  char mqttTopic[60];
  float analogCoef;
  bool apMode;
  bool webServer;
  int sleepTime;
};

struct s_settings g_settings;
StaticJsonDocument<250> doc;

ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);
unsigned long lastMsg = 0;
unsigned long startTime = 0;
int value = 0;
int buttonValue = 0;

DHT dht(DHTPIN, DHTTYPE);
float temperature;
float humidity;
float analogVoltage;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
char timestamp[19];

bool mqtt_connect();
bool readTemperature();
void readVoltage();
void pubData();
void ledBlink(int count);


int EEPROMAnythingWrite(int pos, char *zeichen, int lenge);
int EEPROMAnythingRead(int pos, char *zeichen, int lenge);
int connectToWifi();
int createWifi();

void readSendSleep();

void loadDefaultSettings();
void printSettings();

void handleRoot();
void handleConfig();
void handleCheck();
//void handleSet();
void handleGetConfig();
void handleEditPost();
void handleNotFound();


void setup() {
#ifdef BUTTON_PIN
  pinMode(BUTTON_PIN, INPUT_PULLUP);
#endif

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  EEPROM.begin(512);

  dht.begin();
  timeClient.begin();
  timeClient.setTimeOffset(3600);
  
  startTime = millis();
  Serial.begin(115200);
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("Starting microcontroller...");

  EEPROMAnythingRead(EEPROM_OFFSET, reinterpret_cast<char*>(&g_settings), sizeof(g_settings));
  if(g_settings.c != 's') {
      loadDefaultSettings();
      EEPROMAnythingWrite(EEPROM_OFFSET, reinterpret_cast<char*>(&g_settings), sizeof(g_settings));
  }
  printSettings();

  readTemperature();

  #ifdef BUTTON_PIN
  if(digitalRead(BUTTON_PIN) == LOW) {
    g_settings.webServer = true;
    ledBlink(1);
    delay(2000);
    if(digitalRead(BUTTON_PIN) == LOW) {
      g_settings.apMode = true;
        DEBUG_PRINTLN("Webserver AP mode");
        ledBlink(4);
    } else {
        DEBUG_PRINTLN("Webserver on saved wifi");
        ledBlink(3);
    }
  }
  #endif


  if(g_settings.apMode) {
    createWifi();
  } else {
    if(!connectToWifi()) {
      DEBUG_PRINTLN("Unable to connect to wifi, going to sleep ...");
      digitalWrite(LED_PIN, LOW);
      delay(500);
      ESP.deepSleep(ERROR_SLEEPTIME*1000000);  
    }
  }

  if(!g_settings.webServer) {
    ledBlink(1);
    readSendSleep();
  }

  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/check", handleCheck);
  //server.on("/set", handleSet);
  server.on("/getconfig", handleGetConfig);
  server.on("/editpost", handleEditPost);
  
  server.onNotFound(handleNotFound);
  server.begin();
  DEBUG_PRINTLN("HTTP server started");  
}

void loop()
{
  server.handleClient();

  delay(10);
}

void loadDefaultSettings()
{
  g_settings.c = 's';
  //g_settings.ssid[15];
  //g_settings.passwd[15];
  g_settings.staticIp = false;
  g_settings.analogCoef = 1; //0.100847;
  g_settings.apMode = true;
  g_settings.webServer = true;
  g_settings.sleepTime = 300;
}


void printSettings()
{
  Serial.println("SHOW Config -----------------------");
  Serial.print("c: ");Serial.println(g_settings.c);
  Serial.print("ssid: ");Serial.println(g_settings.ssid);
  Serial.print("passwd: ");Serial.println(g_settings.passwd);
  Serial.print("staticIp: ");Serial.println(g_settings.staticIp);
  Serial.print("mqttIp: ");Serial.println(g_settings.mqttIp);
  Serial.print("mqttTopic: ");Serial.println(g_settings.mqttTopic);
  Serial.print("analogCoef: ");Serial.println(g_settings.analogCoef);
  Serial.print("sleepTime: ");Serial.println(g_settings.sleepTime);
  Serial.print("apMode: ");Serial.println(g_settings.apMode);
  Serial.print("webServer: ");Serial.println(g_settings.webServer);
  Serial.println("END Config ------------------------");
}

int createWifi()
{
  WiFi.disconnect(true); //prevents unreliable client connect (AP / STA colissions error)
  delay(50);
  DEBUG_PRINT("Creating wifi ");
  DEBUG_PRINTLN(AP_SSID);
  WiFi.softAP(AP_SSID, AP_PASS);
  delay(100);
  //WiFi.softAPConfig(ip, ip, subnet);
  //IPAddress ip = WiFi.softAPIP();
  DEBUG_PRINT("AP IP address: ");
  DEBUG_PRINTLN(WiFi.softAPIP());
};

int connectToWifi()
{
  int counter = 0;
  DEBUG_PRINT("Trying to connect to ");
  DEBUG_PRINTLN(g_settings.ssid);
  DEBUG_PRINT("With password ");
  DEBUG_PRINTLN(g_settings.passwd);
  WiFi.mode(WIFI_STA);
  WiFi.begin(g_settings.ssid, g_settings.passwd);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    counter++;
    delay(500);
    DEBUG_PRINT(".");
    if(counter>15)
      return 0;
  }
  DEBUG_PRINT("Connected to ");
  DEBUG_PRINTLN(g_settings.ssid);
  DEBUG_PRINT("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());
  return 1;
}

bool mqtt_connect()
{
  int count = 0;
  while (!mqttClient.connected()) {
    DEBUG_PRINTLN("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str())) {
      DEBUG_PRINTLN("MQTT connected");
    } else {
      count++;
      DEBUG_PRINT("failed, rc=");
      DEBUG_PRINT(mqttClient.state());
      DEBUG_PRINTLN(" try again in 5 seconds");
      if(count > 5) 
      {
        DEBUG_PRINTLN("MQTT login unsuccessfull, stoped trying ...");
        return false;
      }
      delay(500);
    }
  }
  return true;
}

bool readTemperature()
{
  int count = 0;
  DEBUG_PRINT("reading from sensor ");
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  while(isnan(humidity) || isnan(temperature)){
    if(count > SENSOR_RETRIES) {
      DEBUG_PRINTLN();
      temperature = 333;
      humidity = 222;
      return true; // only testing without sensor
      return false;
    }
    DEBUG_PRINT(".");
    delay(500);
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    count++;
  }
  DEBUG_PRINT("T:");DEBUG_PRINT(temperature);
  DEBUG_PRINT(" H:");DEBUG_PRINTLN(humidity);
  return true;
}

void readVoltage()
{
  delay(200);
  analogVoltage = analogRead(A0);
  DEBUG_PRINT("A0 value: ");
  DEBUG_PRINTLN(analogVoltage);
}

void readTimestamp()
{
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime); 
  sprintf(timestamp, "%4d-%02d-%02d %02d:%02d:%02d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);  
}

void pubData()
{
    StaticJsonDocument<150> doc;

    doc["timestamp"] = timestamp;
    doc["value"] = analogVoltage;
    
    doc["inVoltage"] = roundf(100*analogVoltage*ANALOG_READ_CONSTANT)/100;
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    #ifdef BUTTON_PIN
      doc["buttonState"] = buttonValue ? "NOT PRESSED" : "PRESSED"; // press = LOW on BUTTON_PIN
    #endif BUTTON_PIN
    
    char JSONoutput[150];

    serializeJson(doc, JSONoutput);
  
    DEBUG_PRINT("Publish message: ");
    DEBUG_PRINTLN(JSONoutput);
    mqttClient.publish(g_settings.mqttTopic, JSONoutput, true);
    delay(500);
}

void ledBlink(int count)
{
  for(int i=0; i<count;i++)
  {
    digitalWrite(LED_PIN, LOW);
    delay(BLINK_DELAY);
    digitalWrite(LED_PIN, HIGH);
    delay(BLINK_DELAY);
  }
}

void readSendSleep()
{
  mqttClient.setServer(g_settings.mqttIp, 1883);

  if(!mqtt_connect()) {
    DEBUG_PRINTLN("Unable to connect to mqtt server, going to sleep ...");
    digitalWrite(LED_PIN, LOW);
    delay(500);
    ESP.deepSleep(ERROR_SLEEPTIME*1000000);
  }

  readVoltage();
  
  if(!readTemperature()) {
    DEBUG_PRINTLN("Unable to read temperature, going to sleep ...");
    digitalWrite(LED_PIN, LOW);
    delay(500);
    ESP.deepSleep(ERROR_SLEEPTIME*1000000);
  }
 
  #ifdef BUTTON_PIN
    buttonValue = digitalRead(BUTTON_PIN);
  #endif

  readTimestamp();
  
  pubData();

  ledBlink(2);
  
  unsigned long endTime = millis();
  DEBUG_PRINTLN("Miliseconds it took: ");
  DEBUG_PRINTLN(endTime - startTime);
  DEBUG_PRINTLN("Going to sleep now ...");

  ESP.deepSleep(SLEEPTIME*1000000); //micro seconds to seconds
}

void handleRoot()
{
  server.send(200, "text/html", htmlRoot);
}

void handleConfig()
{
  server.send(200, "text/html", htmlConfig);
}

void handleGetConfig()
{
  EEPROMAnythingRead(EEPROM_OFFSET, reinterpret_cast<char*>(&g_settings), sizeof(g_settings));
  readVoltage();
  readTemperature();

  String out = "";
  doc.clear();
  doc["voltage"] = roundf(100*analogVoltage*g_settings.analogCoef)/100;
  doc["temperature"] = 0;//temperature;
  doc["humidity"] = 0;//humidity
   
  doc["ssid"] = g_settings.ssid;
  doc["passwd"] = g_settings.passwd;
  //doc["staticIp"] = g_settings.staticIp;
  doc["mqttIp"] = g_settings.mqttIp;
  doc["mqttTopic"] = g_settings.mqttTopic;
  doc["analogCoef"] = round(10000*g_settings.analogCoef)/10000;
  doc["apMode"] = g_settings.apMode;
  doc["webServer"] = g_settings.webServer;
  doc["sleepTime"] = g_settings.sleepTime;

  serializeJson(doc, out);
  printSettings();

  server.send(200, "application/json", out);
}


void handleCheck()
{ 
  readVoltage();
  readTemperature();
  float voltage = roundf(100*analogVoltage*g_settings.analogCoef)/100;
  String out = "";
  doc.clear();
  doc["voltage"] = voltage;
  doc["temperature"] = 0; //temperature;
  doc["humidity"] = 0; //humidity;

  serializeJson(doc, out);
  
  server.send(200, "application/json", out);
}

void handleEditPost()
{
  String message = "POST form was:\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  DEBUG_PRINTLN(message);

  strcpy(g_settings.ssid, server.arg("ssid").c_str()); 
  strcpy(g_settings.passwd, server.arg("passwd").c_str());
  strcpy(g_settings.mqttIp, server.arg("mqttIp").c_str()); 
  strcpy(g_settings.mqttTopic, server.arg("mqttTopic").c_str()); 
  //g_settings.staticIp = false;
  //strcpy(g_settings.ip, "");
  g_settings.analogCoef = server.arg("analogCoef") ? atof(server.arg("analogCoef").c_str()) : 1; //0.100847;
  g_settings.apMode = server.arg("apMode") == "on" ? 1 : 0;
  g_settings.webServer = server.arg("webServer") == "on" ? 1 : 0;
  g_settings.sleepTime = atoi(server.arg("sleepTime").c_str());

  EEPROMAnythingWrite(EEPROM_OFFSET, reinterpret_cast<char*>(&g_settings), sizeof(g_settings));

  printSettings();

  server.sendHeader("Location", String("/"), true);
  server.send( 302, "text/plain");
}

void handleNotFound()
{
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

int EEPROMAnythingWrite(int pos, char *zeichen, int lenge)
{    
  DEBUG_PRINTLN("eeprom write");
  for (int i = 0; i < lenge; i++)
  {
    EEPROM.write(pos + i, *zeichen);
    zeichen++;
  }
  EEPROM.commit();
  return pos + lenge;
}

int EEPROMAnythingRead(int pos, char *zeichen, int lenge)
{
  for (int i = 0; i < lenge; i++)
  {
    *zeichen = EEPROM.read(pos + i);
    zeichen++;
  }
  return pos + lenge;
}
