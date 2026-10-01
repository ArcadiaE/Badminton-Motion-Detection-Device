#include "HeartRateSpO2.h"

HeartRateSpO2::HeartRateSpO2() {
  bufferIndex = 0;
  heartRate = 0;
  spo2 = 0;
}

void HeartRateSpO2::begin(MAX30102 &sensor, float sampleRate) {
  particleSensor = &sensor;
  samplingRate = sampleRate;
  dt = 1.0 / samplingRate;

  for (int i = 0; i < BUFFER_SIZE; i++) {
    irBuffer[i] = 0;
    redBuffer[i] = 0;
  }
}

bool HeartRateSpO2::update() {
  if (particleSensor->available()) {
    MAX30102Sample sample = particleSensor->readSample();

    if (sample.valid) {
      uint32_t redValue = sample.red;
      uint32_t irValue = sample.ir;

      if (irValue < 50000 || redValue < 50000) {
        // particleSensor->nextSample();
        return false;
      }

      irBuffer[bufferIndex] = irValue;
      redBuffer[bufferIndex] = redValue;
      // bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
      bufferIndex++;

  //     if (bufferIndex == 0) {
  //       calculateHeartRateAndSpO2();
  //       particleSensor->nextSample();
  //       return true;
  //     }
  //   }
  //   particleSensor->nextSample();
  // }
  // return false;

      if (bufferIndex >= BUFFER_SIZE) {
        bufferIndex = 0; 
        calculateHeartRateAndSpO2();
        return true; 
      }
    }
  }
  return false; 
}

float HeartRateSpO2::getHeartRate() {
  return heartRate;
}

float HeartRateSpO2::getSpO2() {
  return spo2;
}

void HeartRateSpO2::calculateHeartRateAndSpO2() {
  applyMovingAverageFilter(irBuffer, BUFFER_SIZE, 4);
  applyMovingAverageFilter(redBuffer, BUFFER_SIZE, 4);

  float irMean = calculateMean(irBuffer, BUFFER_SIZE);
  float redMean = calculateMean(redBuffer, BUFFER_SIZE);

  float irAC = 0;
  float redAC = 0;
  for (int i = 0; i < BUFFER_SIZE; i++) {
    irAC += abs((float)irBuffer[i] - irMean);
    redAC += abs((float)redBuffer[i] - redMean);
  }
  irAC /= BUFFER_SIZE;
  redAC /= BUFFER_SIZE;

  float R = (redAC / redMean) / (irAC / irMean);

  spo2 = 110.0 - 25.0 * R;
  spo2 = constrain(spo2, 0, 100);

  int beatCount = 0;
  for (int i = 1; i < BUFFER_SIZE - 1; i++) {
    if (irBuffer[i] > irBuffer[i - 1] && irBuffer[i] > irBuffer[i + 1] && irBuffer[i] > irMean * 1.2) {
      beatCount++;
    }
  }

  heartRate = (float)beatCount * (60.0 / (BUFFER_SIZE * dt));
}

float HeartRateSpO2::calculateMean(uint32_t *data, int size) {
  uint32_t sum = 0;
  for (int i = 0; i < size; i++) {
    sum += data[i];
  }
  return (float)sum / size;
}

void HeartRateSpO2::applyMovingAverageFilter(uint32_t *data, int size, int windowSize) {
  for (int i = 0; i < size - windowSize + 1; i++) {
    uint32_t sum = 0;
    for (int j = 0; j < windowSize; j++) {
      sum += data[i + j];
    }
    data[i] = sum / windowSize;
  }
}
