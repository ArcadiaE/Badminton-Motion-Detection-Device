#ifndef HEART_RATE_SPO2_H
#define HEART_RATE_SPO2_H

#include <Arduino.h>
#include "MAX30102.h"

class HeartRateSpO2 {
public:
  HeartRateSpO2();

  void begin(MAX30102 &sensor, float sampleRate);
  bool update();
  float getHeartRate();
  float getSpO2();

private:
  MAX30102 *particleSensor;
  float samplingRate;
  float dt;

  static const int BUFFER_SIZE = 100;
  uint32_t irBuffer[BUFFER_SIZE];
  uint32_t redBuffer[BUFFER_SIZE];
  int bufferIndex;

  float heartRate;
  float spo2;

  void calculateHeartRateAndSpO2();
  float calculateMean(uint32_t *data, int size);
  void applyMovingAverageFilter(uint32_t *data, int size, int windowSize);
};

#endif // HEART_RATE_SPO2_H
