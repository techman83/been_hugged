/*
 * 
 * Techman83's Hug Detector
 *
 */

#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <VL6180X.h>
#include <Adafruit_DotStarMatrix.h>
#include <DotMatrix_GrowingHeart.h>
#include <SPI.h>
#include <Hugged.h>

const char ssid[] = WIFI_SSID;
const char pass[] = WIFI_PASS;

WiFiClientSecure net;
MQTTClient client;
VL6180X sensor;

int ledPin = 22;

Adafruit_DotStarMatrix matrix = Adafruit_DotStarMatrix(
  8, 8,  // Width, height
  23, 18, // Data pin, clock pin
  DS_MATRIX_BOTTOM  + DS_MATRIX_LEFT +
  DS_MATRIX_ROWS + DS_MATRIX_PROGRESSIVE,
  DOTSTAR_BRG);

// Feedback loop
uint16_t PIXELS = matrix.numPixels();
uint16_t CUR_INDEX = 0;
uint32_t RED = matrix.Color(255, 0, 0);
uint32_t BLUE = matrix.Color(0, 200, 0);
uint32_t PINK = matrix.Color(200, 200, 0);
uint32_t OFF = matrix.Color(0, 0, 0); //BLACK

DotMatrix_GrowingHeart heart;
Hugged hugged;

void sensor_init()
{
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
  matrix.setPixelColor(CUR_INDEX, OFF);
  if (++CUR_INDEX >= PIXELS) {
    CUR_INDEX = 0;
    matrix.show();
    return false;
  }
  matrix.setPixelColor(CUR_INDEX, colour);
  matrix.show();
  return true;
}


void connect() {
  Serial.print(MQTT_USER);
  Serial.print("\nconnecting...");
  int failure = 0;
  digitalWrite(ledPin, LOW);
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

  matrix.setPixelColor(CUR_INDEX, OFF);
  matrix.show();
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

  matrix.setPixelColor(CUR_INDEX, OFF);
  matrix.show();
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
  heart.setMatrix(&matrix)
    .matrixBegin();
  client.begin(MQTT, 8883, net);
  client.onMessage(messageReceived);

  wifi_connect();
  connect();
  sensor_init();

  while (pixelMove(RED)) {
    delay(25);
  }
  heart.reset();

  hugged.setHugged(&client)
    .setSensor(&sensor)
    .setHeart(&heart);

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
