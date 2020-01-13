/*
 * 
 * Techman83's Hug Detector
 *
 */

#include <Wire.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <VL6180X.h>
#include <Hugged.h>
#include <TinyPICO.h>
#include <Adafruit_DotStar.h>

const char ssid[] = WIFI_SSID;
const char pass[] = WIFI_PASS;

WiFiClientSecure net;
MQTTClient client;
VL6180X sensor;
TinyPICO tp = TinyPICO();

#define NUMPIXELS 72 // Number of LEDs in strip
#define DATAPIN    23
#define CLOCKPIN   18
Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BGR);

// Feedback loop
uint16_t PIXELS = 72;
uint16_t CUR_INDEX = 0;
uint32_t RED = strip.Color(255, 0, 0);
uint32_t BLUE = strip.Color(0, 200, 0);
uint32_t PINK = strip.Color(200, 200, 0);
uint32_t OFF = strip.Color(0, 0, 0); //BLACK

Hugged hugged;

void sensor_init()
{
  Wire.begin();
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

bool pixelMove(uint32_t colour) {
  strip.setPixelColor(CUR_INDEX, OFF);
  if (++CUR_INDEX >= PIXELS) {
    CUR_INDEX = 0;
    strip.setPixelColor(CUR_INDEX, colour);
    strip.show();
    return false;
  }
  strip.setPixelColor(CUR_INDEX, colour);
  strip.show();
  return true;
}


void connect() {
  Serial.print(MQTT_USER);
  Serial.print("\nconnecting...");
  int failure = 0;
  tp.DotStar_CycleColor(25);
  while (!client.connect(CLIENT_ID,MQTT_USER,MQTT_PASS)) {
    pixelMove(PINK);
    Serial.print(".");
    delay(250);
    Serial.print(client.lastError());
    Serial.print(client.returnCode());
    failure += 1;
    if (failure >= 20) {
      ESP.restart();
    }
  }

  tp.DotStar_SetPower( false );
  strip.fill();
  strip.show();
  String conMessage = String(CLIENT_ID) + " connected";
  client.publish("/test", conMessage.c_str() );
  client.subscribe("/heartbeat");
  Serial.println("MQTT connected");
}


void wifi_connect() {
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  Serial.println("...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    pixelMove(BLUE);
    Serial.print(".");
    delay(1000);
  }

  strip.fill();
  strip.show();
  Serial.println("WiFi connected");
}


void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  if ( ! hugged.hugged() && payload == "ping" ) {
    pixelMove(RED);
  }
}


void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.show();
  client.begin(MQTT, 8883, net);
  client.onMessage(messageReceived);

  wifi_connect();
  connect();
  sensor_init();

  while (pixelMove(RED)) {
    delay(25);
  }
  strip.fill();
  strip.show();

  hugged.setHugged(&client)
    .setSensor(&sensor)
    .setStrip(&strip);

  Serial.println("Ready");
}


void loop() {
  client.loop();
  delay(50);  // <- fixes some issues with WiFi stability

  if (WiFi.status() != WL_CONNECTED)
    wifi_connect();
  if (!client.connected())
    connect();

  hugged.hugLoop();
}
