/*
 * 
 * Techman83's Hug Detector
 *
 */

#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <VL6180X.h>

const char ssid[] = WIFI_SSID;
const char pass[] = WIFI_PASS;

WiFiClientSecure net;
MQTTClient client;
VL6180X sensor;

unsigned long huggedTime = 0;
int hugTicks = 0;
int hugsPotential = 0;
int lastRange = 0;
int ledPin = 22;
volatile bool hugStuck = false;
volatile bool hugs = false;

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

void connect() {
  delay(2000);
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print(MQTT_USER);
  Serial.print("\nconnecting...");
  while (!client.connect(CLIENT_ID,MQTT_USER,MQTT_PASS)) {
    Serial.print(".");
    delay(1000);
    Serial.print(client.lastError());
    Serial.print(client.returnCode());
  }

  String conMessage = String(CLIENT_ID) + " connected";
  client.publish("/test", conMessage.c_str() );
  Serial.println("MQTT connected");
}


void wifi_connect() {
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  Serial.println("...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    return;
  }
  Serial.println("WiFi connected");
}


void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
}


void setup() {
  Serial.begin(115200);
  client.begin(MQTT, 8883, net);
  client.onMessage(messageReceived);

  wifi_connect();
  connect();
  sensor_init();
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


void loop() {
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (WiFi.status() != WL_CONNECTED)
    wifi_connect();
  if (!client.connected())
    connect();

  is_it_me();
}
