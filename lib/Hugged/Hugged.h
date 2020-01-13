#ifndef __HUGGED_H
#define __HUGGED_H

#include <cstdlib>
#include <MQTT.h>
#include <VL6180X.h>
#include <Adafruit_DotStar.h>

class Hugged {
private:
  MQTTClient* client;
  VL6180X* sensor;
  Adafruit_DotStar* strip;
  uint16_t pixels;
unsigned long huggedTime;
uint16_t hugTicks;
uint16_t hugsPotential;
uint16_t hugQuality;
uint16_t hugExpiry;
uint16_t lastRange;
uint32_t color;
volatile bool hugStuck;
volatile bool hugState;
void clear_strip();
void clear_hug();
void display_quality();

public:
  Hugged();
  Hugged& setHugged(MQTTClient* client);
  Hugged& setSensor(VL6180X* sensor);
  Hugged& setStrip(Adafruit_DotStar* heart);
  void hugLoop();
  bool hugged();
};

#endif
