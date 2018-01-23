/*
 * Message format is "on/off" like:
 *
 *   high  (defined pin on)
 *   low   (defined pin off)
 *
 */

#include <WiFi.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <VL6180X.h>

unsigned long huggedTime = 0;
int hugTicks = 0;
int hugsPotential = 0;
int lastRange = 0;
int ledPin = 22;
volatile bool hugStuck = false;
volatile bool hugs = false;

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
VL6180X sensor;

/**
 * Setup
 */
void sensor_init()
{
  delay(2000);
  sensor.init();
  sensor.configureDefault();
  Serial.println( "Sensor Config..." );

  // Reduce range max convergence time and ALS integration
  // time to 30 ms and 50 ms, respectively, to allow 10 Hz
  // operation (as suggested by Table 6 ("Interleaved mode
  // limits (10 Hz operation)") in the datasheet).
  sensor.writeReg(VL6180X::SYSRANGE__MAX_CONVERGENCE_TIME, 30);
  sensor.writeReg16Bit(VL6180X::SYSALS__INTEGRATION_PERIOD, 50);

  sensor.setTimeout(500);

   // stop continuous mode if already active
  sensor.stopContinuous();
  // in case stopContinuous() triggered a single-shot
  // measurement, wait for it to complete
  delay(300);
  // start interleaved continuous mode with period of 100 ms
  sensor.startInterleavedContinuous(100);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("WiFi begun");
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Serial.println("Proceeding");
  // Set Pin mode for IR Sensor

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(CLIENT_ID);
  ArduinoOTA.setPassword(OTA_PASS);

  client.setServer(MQTT, 12839);
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
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  sensor_init();
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  delay(500);
  digitalWrite(ledPin, HIGH);
  delay(500);
  Serial.println("Ready");
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
  if (hugStuck)
    return;

  unsigned long timeNow = millis();
  if (timeNow >= huggedTime) {
    int range = sensor.readRangeContinuousMillimeters();
    lastRange = range;

    if (sensor.timeoutOccurred()) 
    { 
      Serial.print(" TIMEOUT");
      huggedTime = timeNow + 50;
      return;
    }

    // (Potential)? Hug cleared
    if (hugTicks == 0 && hugs == true)
    {
      hugs = false;
      hugsPotential = 0;
      digitalWrite(ledPin, HIGH);
      Serial.println("(Potential)? Hug cleared");
    }
   
    // I've been hugged!
    if (hugTicks == 7 && hugs == false)
    {
      hugs = true;
      Serial.println("I've been hugged <3");
      client.publish("/hugged", "true");
      digitalWrite(ledPin, LOW);
      huggedTime = timeNow + 10000;
      return;
    }

    // Oh noes I'm potentially stuck in a hug!
    // (more likely my badge is obstructed... or my hacky code)
    if (hugTicks == 200)
    {
      hugStuck = true;
      Serial.println("Ack! I'm stuck!");
      client.publish("/stuck", "true");
      return;
    }

    if (range < 200 && lastRange + 10 >= range && hugsPotential < 6) {
      hugsPotential +=1;
      Serial.println("Pontential Hug tick increase");
    }
    
    if( range > 200 && hugsPotential > 0)
    {
      Serial.println("Pontential Hug tick decrease");
      hugsPotential -= 1;
    }

    if( range < 100 && hugsPotential > 5)
    {
      Serial.println("Hug tick increase");
      hugTicks += 1;

    }
    if( range > 150 && hugTicks > 0)
    {
      Serial.println("Hug tick decrease");
      hugTicks -= 1;
    }
    //Serial.println("tick end");
    huggedTime = timeNow + 50;
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

  // Call delay periodically to avoid triggering the watchdog
  delay(10);
}
