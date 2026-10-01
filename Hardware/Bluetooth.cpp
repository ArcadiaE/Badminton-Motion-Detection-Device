#include "Bluetooth.h"

// service UUID & feature UUID
#define SENSOR_SERVICE_UUID        "19B10000-E8F2-537E-4F6C-D104768A1214"       
#define ROLL_CHARACTERISTIC_UUID   "19B10001-E8F2-537E-4F6C-D104768A1214"      
#define PITCH_CHARACTERISTIC_UUID  "19B10002-E8F2-537E-4F6C-D104768A1214"
#define YAW_CHARACTERISTIC_UUID  "19B10003-E8F2-537E-4F6C-D104768A1214"
#define HEART_RATE_CHARACTERISTIC_UUID "19B10004-E8F2-537E-4F6C-D104768A1214"
#define SPO2_CHARACTERISTIC_UUID   "19B10005-E8F2-537E-4F6C-D104768A1214"

BLEService sensorService(SENSOR_SERVICE_UUID);

BLEFloatCharacteristic rollCharacteristic(ROLL_CHARACTERISTIC_UUID, BLERead | BLENotify);
BLEFloatCharacteristic pitchCharacteristic(PITCH_CHARACTERISTIC_UUID, BLERead | BLENotify);
BLEFloatCharacteristic yawCharacteristic(YAW_CHARACTERISTIC_UUID, BLERead | BLENotify);
BLEFloatCharacteristic heartRateCharacteristic(HEART_RATE_CHARACTERISTIC_UUID, BLERead | BLENotify);
BLEFloatCharacteristic spo2Characteristic(SPO2_CHARACTERISTIC_UUID, BLERead | BLENotify);

BLEDevice central;

// BLE initialization
void initBLE() {
  if (!BLE.begin()) {
    Serial.println("BLE initialization failed");
    while (1);
  }
  Serial.println("BLE initialization succeed");
  

  BLE.setDeviceName("TOGENASHI_TOGEARI");
  BLE.setLocalName("TOGENASHI_TOGEARI");
  BLE.setAdvertisedService(sensorService); 

  sensorService.addCharacteristic(rollCharacteristic);
  sensorService.addCharacteristic(pitchCharacteristic);
  sensorService.addCharacteristic(yawCharacteristic);
  sensorService.addCharacteristic(heartRateCharacteristic);
  sensorService.addCharacteristic(spo2Characteristic);

  BLE.addService(sensorService);

  rollCharacteristic.writeValue(0.0);
  pitchCharacteristic.writeValue(0.0);
  yawCharacteristic.writeValue(0.0);
  heartRateCharacteristic.writeValue(0.0);
  spo2Characteristic.writeValue(0.0);

  BLE.advertise();
  Serial.println("BLE established, waiting for connection...");
}

void updateBLECharacteristics(float roll, float pitch, float yaw, float heartRate, float spo2) {
  rollCharacteristic.writeValue(roll);
  pitchCharacteristic.writeValue(pitch);
  yawCharacteristic.writeValue(yaw);
  heartRateCharacteristic.writeValue(heartRate);
  spo2Characteristic.writeValue(spo2);
}

void pollBLE() {
  //FBI watching!
  BLEDevice newCentral = BLE.central();

  if (newCentral && !central) {
    central = newCentral;
    Serial.print("Device Connected: ");
    Serial.println(central.address());
  }

  if (central && !central.connected()) {
    Serial.print("Device Disconnected: ");
    Serial.println(central.address());
    central = BLEDevice(); // 这里进行了修改，原本是 central = 0
  }

  BLE.poll();
}
