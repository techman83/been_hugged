#ifndef __HUGGED_H
#define __HUGGED_H

#include <cstdlib>
#include <MQTT.h>
#include <VL6180X.h>
#include <DotMatrix_GrowingHeart.h>

class Hugged {
private:
  MQTTClient* client;
  VL6180X* sensor;
  DotMatrix_GrowingHeart* heart;
unsigned long huggedTime;
uint16_t hugTicks;
uint16_t hugsPotential;
uint16_t hugQuality;
uint16_t hugExpiry;
uint16_t lastRange;
volatile bool hugStuck;
volatile bool hugState;

public:
  Hugged();
  Hugged& setHugged(MQTTClient* client);
  Hugged& setSensor(VL6180X* sensor);
  Hugged& setHeart(DotMatrix_GrowingHeart* heart);
  void hugLoop();
  bool hugged();
};

#endif
