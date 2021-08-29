/*
  SimpleMQTTClient.ino
  The purpose of this exemple is to illustrate a simple handling of MQTT and Wifi connection.
  Once it connects successfully to a Wifi network and a MQTT broker, it subscribe to a topic and send a message to it.
  It will also send a message delayed 5 seconds later.
*/

#include "EspMQTTClient.h"
#include <ArduinoJson.h>
#include "ft817.h"
#include "globals.h"

extern bool  current_vfo;
extern byte current_mode;
extern byte current_band;
extern byte current_agc;
extern byte current_pwr;
extern byte current_chg;
extern bool charging;
extern byte current_ssb_step;
extern byte current_am_step;
extern byte current_fm_step;
extern bool locked;
extern byte txStatus;
extern byte current_ant;
extern char modes[13][4];
extern char bands[16][6];
extern char agc[4][5];
extern long ssbSteps[3];
extern long amSteps[6];
extern long fmSteps[8];
extern char pwr[4][5];
extern char chargeTimes[3][4];

extern bool process_commands(char *);


extern FT817 radio;

void send_mqtt();

EspMQTTClient mqtt(
  "<YOUR SSID>",
  "<YOUR WPA PASSWORD>",
  "<YOUR MQTT BROKER IP ADDRESS>",
  "<USERNAME>",   // Can be omitted if not needed
  "<PASSWORD>",   // Can be omitted if not needed
  "FT817",     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);

void setup_mqtt()
{
  #ifndef MQTT
  return;
  #endif

  // Optionnal functionnalities of EspMQTTClient : 
  mqtt.enableDebuggingMessages(false); // Enable debugging messages sent to serial output
  mqtt.enableHTTPWebUpdater(); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overrited with enableHTTPWebUpdater("user", "password").
  mqtt.enableLastWillMessage("FT817/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true

  mqtt.setMaxPacketSize(512); // max. length of payload
}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished()
{
  // Subscribe to "FT817/frequency" and display received message to Serial
  mqtt.subscribe("FT817/frequency", [](const String & payload) {
    Serial.println(payload);
  });

  // Subscribe to "FT817/settings" and display received message to Serial
  mqtt.subscribe("FT817/command", [](const String & payload) {

    char buffer[100];
    int bufSiz = sizeof(buffer) / sizeof(buffer[0]);
    payload.toCharArray(buffer, bufSiz-1);
    process_commands(buffer);

  });

/*
  // Subscribe to "mytopic/wildcardtest/#" and display received message to Serial
  mqtt.subscribe("FT817/test1/#", [](const String & topic, const String & payload) {
    Serial.println("(From wildcard) topic: " + topic + ", payload: " + payload);
  });

  // Publish a message to "mytopic/test"
  mqtt.publish("FT817/test2", "This is a message"); // You can activate the retain flag by setting the third parameter to true

  // Execute delayed instructions
  mqtt.executeDelayed(5 * 1000, []() {
    mqtt.publish("FT817/wildcardtest/test123", "This is a message sent 5 seconds later");
  });
*/

}

void send_mqtt(char *name, char *value_str) {

  char json_str[128];
  StaticJsonDocument<128> doc;

#ifndef MQTT
return;
#endif

  doc[name] = value_str;
  serializeJsonPretty(doc, json_str);
  //mqtt.publish("FT817/settings", json_str); 
  
  // Instead, just for test:
  char buf[30];
  if(strcmp(name,"frequency") == 0 || strcmp(name,"smeter") == 0)
    strcpy(buf,"FT817/status");
 //   sprintf(buf,"FT817/status/%s",name);
  else
    strcpy(buf,"FT817/settings");
 //   sprintf(buf,"FT817/settings/%s",name);
  mqtt.publish(buf, json_str); 
  // end testing

}


void send_mqtt_ant(char *name, byte value) {

  char json_str[128];
  StaticJsonDocument<128> doc;
  JsonArray data = doc.createNestedArray(name);

#ifndef MQTT
return;
#endif

  for(int i=0;i<6;i++) {  // 0=HF, 1=6m, 2=BCR, 3=Air, 4=2m, 5=UHF
    if( bitRead(value,i) )
      data.add("REAR");	
    else
      data.add("FRONT");	
  }
  serializeJsonPretty(doc, json_str);
  //mqtt.publish("FT817/settings", json_str); 
  // Instead, just for test:
  char buf[30];
  sprintf(buf,"FT817/settings/%s",name);
  mqtt.publish(buf, json_str); 
  // end testing
}