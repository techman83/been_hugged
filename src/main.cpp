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

const char ssid[] = WIFI_SSID;
const char pass[] = WIFI_PASS;

WiFiClientSecure net;
MQTTClient client;
VL6180X sensor;

unsigned long huggedTime = 0;
uint16_t hugTicks = 0;
uint16_t hugsPotential = 0;
uint16_t hugQuality = 0;
uint16_t hugExpiry = 0;
uint16_t lastRange = 0;
int ledPin = 22;
volatile bool hugStuck;
volatile bool hugs = false;

Adafruit_DotStarMatrix matrix = Adafruit_DotStarMatrix(
  8, 8,  // Width, height
  23, 18, // Data pin, clock pin
  DS_MATRIX_BOTTOM  + DS_MATRIX_LEFT +
  DS_MATRIX_ROWS + DS_MATRIX_PROGRESSIVE,
  DOTSTAR_BRG);

// Feedback loop
uint16_t PIXELS = matrix.numPixels();
uint16_t CUR_INDEX = 0;
uint32_t COLOUR = matrix.Color(255, 0, 0); //RED
uint32_t OFF = matrix.Color(0, 0, 0); //BLACK

DotMatrix_GrowingHeart heart;

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

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print(MQTT_USER);
  Serial.print("\nconnecting...");
  int failure = 0;
  digitalWrite(ledPin, LOW);
  while (!client.connect(CLIENT_ID,MQTT_USER,MQTT_PASS)) {
    digitalWrite(ledPin, LOW);
    Serial.print(".");
    delay(1000);
    digitalWrite(ledPin, HIGH);
    Serial.print(client.lastError());
    Serial.print(client.returnCode());
    failure += 1;
    if (failure >= 10) {
      ESP.restart();
    }
  }

  String conMessage = String(CLIENT_ID) + " connected";
  client.publish("/test", conMessage.c_str() );
  client.subscribe("/heartbeat");
  digitalWrite(ledPin, HIGH);
  Serial.println("MQTT connected");
}


void wifi_connect() {
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  Serial.println("...");
  digitalWrite(ledPin, LOW);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  digitalWrite(ledPin, HIGH);

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    return;
  }
  Serial.println("WiFi connected");
}


bool pixelMove() {
  matrix.setPixelColor(CUR_INDEX, OFF);
  if (++CUR_INDEX >= PIXELS) {
    CUR_INDEX = 0;
    matrix.show();
    return false;
  }
  matrix.setPixelColor(CUR_INDEX, COLOUR);
  matrix.show();
  return true;
}


void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  if ( ! hugs  && payload == "ping" ) {
    pixelMove();
  }
}


void setup() {
  Serial.begin(115200);
  delay(2000); // wait for device to settle
  heart.setMatrix(&matrix)
    .matrixBegin();
  client.begin(MQTT, 8883, net);
  client.onMessage(messageReceived);

  wifi_connect();
  connect();
  sensor_init();

  while (pixelMove()) {
    delay(25);
  }
  heart.reset();

  Serial.println("Ready");
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

    // Publish and Clear Hug
    if (hugs == true && hugExpiry >= 10) {
      Serial.print("Hug Quality (");
      Serial.print(hugQuality);
      Serial.println(") Measured and Published");
      hugs = false;
      hugsPotential = 0;
      hugExpiry = 0;
      client.publish("/hugged", String(hugQuality));
      Serial.println("Hug cleared");
      delay(10000);
      heart.reset();
      // This is an awful hack, until I figure out why `hugs` reverts to true
      // on the next loop.
      ESP.restart();
      return;
    }

    // Measuring quality
    if (hugs == true && range < 100) {
      hugQuality += 1;
      heart.increase();
      Serial.println("Hug Quality increase");
      huggedTime = timeNow + 25;
      return;
    }

    if (hugs == true) {
      hugExpiry += 1;
      Serial.println("Expiry Timeout Increasing");
      huggedTime = timeNow + 25;
      return;
    }

    // I've been hugged!
    if (hugTicks >= 7 && hugs == false)
    {
      hugs = true;
      heart.reset();
      Serial.println("I've been hugged <3");
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
    huggedTime = timeNow + 25;
  }
}


void loop() {
  client.loop();
  delay(50);  // <- fixes some issues with WiFi stability

  if (WiFi.status() != WL_CONNECTED)
    wifi_connect();
  if (!client.connected())
    connect();

  is_it_me();
}
