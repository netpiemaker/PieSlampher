#include <AuthClient.h>
#include <MicroGear.h>
#include <MQTTClient.h>
#include <SHA1.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <MicroGear.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#define RELAYPIN 12
#define LEDPIN 13
#define BUTTONPIN D3
#define EEPROM_STATE_ADDRESS 128

#define APPID   <APPID>
#define KEY     <APPKEY>
#define SECRET  <APPSECRET>
#define ALIAS   "pieslampher"

WiFiClient client;
AuthClient *authclient;

char state = 0;
char tosendstate = 0;
char buff[16];

MicroGear microgear(client);

void sendState() {
  tosendstate = 0;
  sprintf(buff,"%d",state);
  microgear.publish("/pieslampher/state",buff);
}

void updateIO() {
  if (state >= 1) {
    digitalWrite(RELAYPIN, HIGH);
    digitalWrite(LEDPIN, LOW);

    EEPROM.write(EEPROM_STATE_ADDRESS, 1);
    EEPROM.commit();
  }
  else {
    state = 0;
    digitalWrite(RELAYPIN, LOW);
    digitalWrite(LEDPIN, HIGH);

    EEPROM.write(EEPROM_STATE_ADDRESS, 0);
    EEPROM.commit();
  }
}

void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
  int s;
  char *m = (char *)msg;
  m[msglen] = '\0';

  Serial.println(m);
  s = atoi(m);
  
  if (s == 1) state = 1;
  else state = 0;

  updateIO();
  tosendstate = 1;
}

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.println("Connected to NETPIE...");
  tosendstate = 1;
}

static void onButtonPressed(void) {
  Serial.println("Button Pressed");
  if (state == 1) state = 0;
  else state = 1;
  
  updateIO();
  tosendstate = 1;
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting...");

    pinMode(BUTTONPIN, INPUT);
    pinMode(RELAYPIN, OUTPUT);
    pinMode(LEDPIN, OUTPUT);

    attachInterrupt( BUTTONPIN, onButtonPressed, FALLING );

    EEPROM.begin(512);
    state = EEPROM.read(EEPROM_STATE_ADDRESS)==1?1:0;
    updateIO();

    WiFiManager wifiManager;
    wifiManager.setTimeout(180);

    if(!wifiManager.autoConnect("PieSlampher")) {
      Serial.println("Failed to connect and hit timeout");
      delay(3000);
      ESP.reset();
      delay(5000);
    }
  
    microgear.on(MESSAGE,onMsghandler);
    microgear.on(CONNECTED,onConnected);

    microgear.init(KEY,SECRET,ALIAS);
    microgear.connect(APPID);
}

void loop() {
  if (microgear.connected()) {
    if (tosendstate) sendState();
    microgear.loop();
  }
  else {
    Serial.println("connection lost, reconnect...");
    microgear.connect(APPID);
  }
}

