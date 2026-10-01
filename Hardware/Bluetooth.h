#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <ArduinoBLE.h> 

void initBLE();
void updateBLECharacteristics(float roll, float pitch, float yaw, float heartRate, float spo2);
// void updateBLECharacteristics(float roll, float pitch);
void pollBLE();

extern BLEService sensorService;

extern BLEFloatCharacteristic rollCharacteristic;
extern BLEFloatCharacteristic pitchCharacteristic;
extern BLEFloatCharacteristic yawCharacteristic;
extern BLEFloatCharacteristic heartRateCharacteristic;
extern BLEFloatCharacteristic spo2Characteristic;

extern BLEDevice central;

#endif
