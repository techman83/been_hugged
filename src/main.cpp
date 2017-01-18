/*
 * Message format is "on/off" like:
 *
 *   high  (defined pin on)
 *   low   (defined pin off)
 *
 */

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

/* Init state globals */
uint8_t PROX = D5;
volatile bool stuck = false;

/* MQTT Settings */
#define BUFFER_SIZE 100
char message_buff[100];

/* ***************************************************** */
/**
 * MQTT callback to process messages
 */

//print any message received for subscribed topic
void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  int i = 0;
  for (i=0;i < length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';

  String state = String(message_buff);
  state.toUpperCase();
  Serial.println(state);
}

WiFiClient wificlient;
PubSubClient client(wificlient);

/**
 * Setup
 */
void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.hostname(CLIENT_ID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("WiFi begun");
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  pinMode(PROX, INPUT);

  Serial.println("Proceeding");

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(CLIENT_ID);
  //ArduinoOTA.setPassword(OTA_PASS);

  client.setServer(MQTT, 1883);
  client.setCallback(callback);
  
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if      (error == OTA_AUTH_ERROR   ) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR  ) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR    ) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(CLIENT_ID,MQTT_USER,MQTT_PASS)) {
      Serial.println("connected");

      // Once connected, publish an announcement...
      String conMessage = String(CLIENT_ID) + " connected";
      client.publish("/test", conMessage.c_str() );

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void is_it_me() { 
  if(stuck == true)
    return;  

  int state = digitalRead(PROX);
  int count = 0;
  while (count < 30 && state == 0) {
    if (count == 5) {
      Serial.println("I've been hugged");
      client.publish("/hugged", "true");
    }
    
    if (count > 25) {
      Serial.println("Hrm is this just a long hug??");
      stuck = true;
      client.publish("/stuck", "true");
      break;
    }
     
    state = digitalRead(PROX);
    count = count + 1;
    delay(1000);
  }
}


/**
 * Main
 */
void loop() {
  ArduinoOTA.handle();
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Connecting to ");
    Serial.print(WIFI_SSID);
    Serial.println("...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    if (WiFi.waitForConnectResult() != WL_CONNECTED)
      return;
    Serial.println("WiFi connected");
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      reconnect();
    }
  }


  is_it_me();

  if (client.connected())
    client.loop();
}
