/*
 * Message format is "on/off" like:
 *
 *   high  (defined pin on)
 *   low   (defined pin off)
 *
 */

#define FASTLED_ESP8266_D1_PIN_ORDER
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <FastLED.h>


#define IR_PIN D0
unsigned long huggedTime = 0;
int hugTicks = 0;
volatile bool hugStuck = false;

// WS2812 / FASTLED Config
#define NUM_LEDS 2
#define DATA_PIN  D2
uint32_t ledPosition = 0;
volatile boolean hugs = false;
CRGB leds[NUM_LEDS];    // Define the array of leds

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
void led_startup() {
  FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS);

  // Scan red, then green, then blue across the LEDs
  for(int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB::Red;
    FastLED.show();
    delay(200);
    leds[i] = CRGB::Black;
    FastLED.show();
  }
  for(int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB::Green;
    FastLED.show();
    delay(200);
    leds[i] = CRGB::Black;
    FastLED.show();
  }
  for(int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB::Blue;
    FastLED.show();
    delay(200);
    leds[i] = CRGB::Black;
    FastLED.show();
  }
}

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

  Serial.println("Proceeding");
  // Set Pin mode for IR Sensor
  pinMode(IR_PIN, INPUT);

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
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  led_startup();
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
  unsigned long timeNow = millis();
  if (timeNow >= huggedTime && hugStuck == false) {
    //Serial.println("tick start");
    int ir = digitalRead(IR_PIN);

    // (Potential)? Hug cleared
    if (hugTicks == 0 && hugs == true)
    {
      hugs = false;
      int i;
      Serial.println("(Potential)? Hug cleared");
      for(i = 0; i < NUM_LEDS; i++)
      {
        leds[i] = CRGB::Black;
        FastLED.show();
      }
    }

    // I've been hugged!
    if (hugTicks == 8 && hugs == false)
    {
      hugs = true;
      Serial.println("I've been hugged <3");
      client.publish("/hugged", "true");
    }

    // Oh noes I'm potentially stuck in a hug!
    // (more likely my badge is obstructed... or my hacky code)
    if (hugTicks == 50)
    {
      int i;
      CHSV hsv( 0, 255, 200);
      for(i = 0; i < NUM_LEDS; i++) {
        leds[i] = hsv;
      	FastLED.show();
      }

      hugStuck = true;
      Serial.println("Ack! I'm stuck!");
      client.publish("/stuck", "true");
    }

    if( ir == 0 )
    {
      Serial.println("Hug tick increase");
      hugTicks += 1;

      int brightness;
      if (hugTicks < 20)
      {
        brightness = hugTicks * 10;
      }
      else
      {
        brightness = 200;
      }
     
      // @projectgus debugged my code. Iterating past the end of the list
      // will corrupt memory.
      int i;
      CHSV hsv( 160, 255, brightness);
      for(i = 0; i < NUM_LEDS; i++)
      {
        leds[i] = hsv;
        FastLED.show();
      }
    }
    if( ir == 1 && hugTicks > 0)
    {
      Serial.println("Hug tick decrease");
      hugTicks -= 1;
    }
    //Serial.println("tick end");
    huggedTime = timeNow + 100;
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
