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


#define RANGE_PIN A0
char GP2D12;
char a,b;
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
  for(int i = 0; i <= NUM_LEDS; i++)
  {
    leds[i] = CRGB::Red;
    FastLED.show();
    delay(200);
    leds[i] = CRGB::Black;
    FastLED.show();
  }
  for(int i = 0; i <= NUM_LEDS; i++)
  {
    leds[i] = CRGB::Green;
    FastLED.show();
    delay(200);
    leds[i] = CRGB::Black;
    FastLED.show();
  }
  for(int i = 0; i <= NUM_LEDS; i++)
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
  led_startup();
  delay(2000);
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

float read_gp2d12_range(byte pin)
{
  int tmp;
  tmp = analogRead(pin);
  if (tmp < 3)return -1;
  return (13574.0 /((float)tmp - 3.0)) - 4.0;
}

void is_it_me() {
  unsigned long timeNow = millis();
  if (timeNow >= huggedTime && hugStuck == false) {
    huggedTime = timeNow + 100;
    int val;
    int i;
    GP2D12=read_gp2d12_range(RANGE_PIN);
    a=GP2D12/10;
    b=GP2D12%10;
    val=a*10+b;

    if(val>10 && val<80)
    {
      Serial.print(a,DEC);
      Serial.print(b,DEC);
      Serial.println("cm");
    }

    // (Potential)? Hug cleared
    if (val>30)
    {
      Serial.println("(Potential)? Hug cleared");
      hugs = false;
      hugTicks = 0;
      for(i = 0; i <= NUM_LEDS; i++)
      {
        leds[i] = CRGB::Black;
        FastLED.show();
      }
    }

    // I've been hugged!
    if (hugTicks == 15)
    {
      Serial.println("I've been hugged <3");
      client.publish("/hugged", "true");
    }

    // Oh noes I'm potentially stuck in a hug!
    // (more likely my badge is obstructed... or my hacky code)
    if (hugTicks == 100)
    {
      CHSV hsv( 0, 255, 255);
      for(i = 0; i <= NUM_LEDS; i++)
      {
        leds[i] = hsv;
        FastLED.show();
      }
      Serial.println("Ack! I'm stuck!");
      client.publish("/stuck", "true");
    }

    if (val>=15 && val<25 )
    {
      Serial.println("Potential hug inbound");
      hugs = true;
    }

    if(hugs == true && val<15)
    {
      Serial.println("Hug tick");
      hugTicks += 1;

      int saturation;
      if (hugTicks < 50)
      {
        saturation = hugTicks * 5;
      }
      else
      {
        saturation = 255;
      }

      CHSV hsv( 160, saturation, 255);
      for(i = 0; i <= NUM_LEDS; i++)
      {
        leds[i] = hsv;
        FastLED.show();
      }
      // As soon as I call FastLED.show hugs reverts to false.
      hugs = true; // TODO: I don't even know why this is necessary
    }
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
