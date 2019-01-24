/**
 * Main Hug Logic for Techman83's Hug Detector
 */

#include <Hugged.h>
#include <Arduino.h>

Hugged::Hugged() {
  this->client        = nullptr;
  this->sensor        = nullptr;
  this->heart         = nullptr;
  this->huggedTime    = 0;
  this->hugTicks      = 0;
  this->hugsPotential = 0;
  this->hugQuality    = 0;
  this->lastRange     = 0;
  this->hugStuck      = false;
  this->hugState      = false;
}

Hugged& Hugged::setHugged(MQTTClient* client) {
  if (client != nullptr) {
    this->client = client;
  }
  return *this;
}

Hugged& Hugged::setSensor(VL6180X* sensor) {
  if (sensor != nullptr) {
    this->sensor = sensor;
  }
  return *this;
}

Hugged& Hugged::setHeart(DotMatrix_GrowingHeart* heart) {
  if (heart != nullptr) {
    this->heart = heart;
  }
  return *this;
}

void Hugged::hugLoop() {
  if (hugStuck)
    return;

  unsigned long timeNow = millis();
  if (timeNow < huggedTime) {
    return;
  }

  // This causes a bug later on!
  int range = sensor->readRangeContinuousMillimeters();
  lastRange = range;

  if (sensor->timeoutOccurred())
  {
    Serial.print(" TIMEOUT");
    huggedTime = timeNow + 50;
    return;
  }

  // Publish and Clear Hug
  if (hugState) {
    if (hugExpiry >= 10) {
      Serial.print("Hug Quality (");
      Serial.print(hugQuality);
      Serial.println(") Measured and Published");
      hugState = false;
      hugsPotential = 0;
      hugExpiry = 0;
      client->publish("/hugged", String(hugQuality));
      Serial.println("Hug cleared");
      delay(7000);
      heart->reset();
      // This is an awful hack, until I figure out why `hugs` reverts to true
      // on the next loop.
      // ESP.restart();
    } else if (range < 100) {
      hugQuality += 1;
      heart->increase();
      Serial.println("Hug Quality increase");
      huggedTime = timeNow + 25;
    } else {
      hugExpiry += 1;
      Serial.println("Expiry Timeout Increasing");
      huggedTime = timeNow + 25;
    }
  } else {
    // hugState is false
    // Note: Once hugTicks is over 7, it can never decrease!
    if (hugTicks >= 200) {
      // Oh noes I'm potentially stuck in a hug!
      // (more likely my badge is obstructed... or my hacky code)
      hugStuck = true;
      Serial.println("Ack! I'm stuck!");
      client->publish("/stuck", "true");
    } else if (hugTicks >= 7) {
      hugState = true;
      heart->reset();
      Serial.println("I've been hugged <3");
    } else {

      // `lastRange + 10 >= range` is always true because `range == lastRange`
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
}

bool Hugged::hugged() {
  return hugState;
}
