#include <ESP8266WiFi.h>
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
char stateOutdated = 0;
char buff[16];

MicroGear microgear(client);

void sendState() {
  if (state==0)
    microgear.publish("/pieslampher/state","0");
  else
    microgear.publish("/pieslampher/state","1");
  Serial.println("send state..");
  stateOutdated = 0;
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
  char m = *(char *)msg;
  
  Serial.print("Incoming message --> ");
  msg[msglen] = '\0';
  Serial.println((char *)msg);
  
  if (m == '0' || m == '1') {
    state = m=='0'?0:1;
    EEPROM.write(EEPROM_STATE_ADDRESS, state);
    EEPROM.commit();
    updateIO();
  }
  if (m == '0' || m == '1' || m == '?') stateOutdated = 1;
}

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.println("Connected to NETPIE...");
  stateOutdated = 1;
}

static void onButtonPressed(void) {
  Serial.println("Button Pressed");
  if (state == 1) state = 0;
  else state = 1;
  
  updateIO();
  stateOutdated = 1;
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
 
    //microgear.resetToken();
    
    microgear.init(KEY,SECRET,ALIAS);
    microgear.connect(APPID);
}

void loop() {
  if (microgear.connected()) {
    if (stateOutdated) sendState();
    microgear.loop();
  }
  else {
    Serial.println("connection lost, reconnect...");
    microgear.connect(APPID);
  }
}

