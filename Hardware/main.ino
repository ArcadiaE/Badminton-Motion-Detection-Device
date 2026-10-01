// 适度编程益脑，过度编程伤身
// 合理安排时间，享受健康生活

// Copyright (c) 2024 Yike Zhang (Arcadia_Ebendie)
// All rights reserved.

// This software is the confidential and proprietary information of Yike Zhang
// ("Confidential Information"). You shall not disclose such Confidential Information
// and shall use it only in accordance with the terms of the license agreement you entered into
// with Yike Zhang.

// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions, and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions, and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// 3. Neither the name of Yike Zhang nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//     .::-------------------------------:---------==-=-     
//     ::-=+++++++++*+++++++++=++++=++++*+++++++++***#*##-    
//    .:-==++++=+=+**++++++++++++++-+++++*+++++******####*.   
//   .:-=+=++++++++*+++++++++++++++::++++*++++*+*****#####+   
//   ::-=++++++++++*+++++++++++++++:.=+++*++++********#####-  
//   :-=+++++++++++*+++++++++++++++:..++**************#####*  
//  .:-=++++++++++++++++*****++++++...-+++*+**+*******######: 
//  ::-+++++++++++++++++++++++++++=....++**+++++******######- 
//  ::=+++++++++++++++++*+++++++++=....=+++=+++++*****######= 
//  ::=+++++++++++++++++++++++++++-....:+++-++++=+*****#####+ 
//  ::+++++++++++++++++*++++++++++:.....+++.++++-+*****#####+ 
//  ::++++++++++++++++++++++*+++++......=+*.=***+******#####+ 
//  ::=++++++++++++++++++++++*+++=......-*#*###**%#****#####- 
//  .:=+++++++++++++*##########*+-......=+%%***=:+#%#**#####. 
//  .:-++++++++++++##*+*++++**+*+:........=%%#*#. .--*#####:  
//   :-+++++++++++##++==+##+#*++=........-*#%#:+. ..-######   
//   ::++++++++++++*++=#+#%+-*............-:-::: ...=######   
//   :-++*++++++++++-:.+:::.:: ............:........*######=  
//  :-++*++*+++++++=.. .:..........................-########+ 
//  -=++*++******++=...............................+######### 
//  =+++*++*+*******:.............................-########## 
//  ++++*++*+++++***:............................-+########## 
//  ++++**+*+++++*++-...........................::-#########* 
//  *+=+**+***+++*+++=...........................-*######%##- 
//  *%==+**********+***:.......................:+###**##%@%@+ 
//  -%%=-+*+**+*********+-...................:+###***#%@@@@@+ 
//  +*%@#+=++++++**********=:..............:+###%%%@@@@@@@@@+ 
//  *@@@@@%-=+++++#++=+**#*#**::.........-*#%%%@@@@@@@@@@@@@= 
//  *@@@@@@--+++++*++=-=*%##***::::::..:---+%%%%@@@@@@@@@@@@= 

#include "Arduino_BMI270_BMM150.h" 
#include "Wire.h"                   
#include "Filter.h"                 
#include "Bluetooth.h"

#include "MadgwickAHRS.h"

#include "MAX30102.h"

#include "HeartRateSpO2.h"

// MAX30102
MAX30102 particleSensor;

//不嘻嘻
HeartRateSpO2 hrSpO2Calculator;

Madgwick filter;

float ax, ay, az; // 加速度计
float gx, gy, gz; // 陀螺仪
float mx, my, mz; // 磁力计

float roll = 0, pitch = 0, yaw = 0;

float dt = 0.005;    // s^(-1)
float alpha = 0.98; // 权重参数，测试后调整

// float rollAlpha = 0.1;
// float pitchAlpha = 0.3;
// float yawAlpha = 0.5; //权重参数，测试后根据实际情况调整！！！
// //如果三种姿态函数使用不一样的alpha

// long irValue, redValue;
float heartRate = 0;
float spo2 = 0;

void i2cScanner() {
  Serial.println("Scanning for I2C devices...");
  byte count = 0;
  for (byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C Device at 0x");
      if (i < 16) Serial.print("0");
      Serial.print(i, HEX);
      Serial.println(" !");
      count++;
      delay(1);
    }
  }
  if (count == 0) {
    Serial.println("No I2C devices found.");
  } else {
    Serial.print("Found ");
    Serial.print(count);
    Serial.println(" device(s).");
  }
}

void readAndFuseIMUData() {
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);
  } 
  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(gx, gy, gz);
  }
  if (IMU.magneticFieldAvailable()) {
    IMU.readMagneticField(mx, my, mz);
  }
  if (!isValidIMUData(ax, ay, az, gx, gy, gz, mx, my, mz)) {
    Serial.println("Not valid data, loading failed!");
    return;
  }


  float accThreshold = 1.0;
  if (abs(ax) < accThreshold && abs(ay) < accThreshold && abs(az) < accThreshold) {
    return;
  }

  float gyroThreshold = 0.1;
  if (abs(gx) < gyroThreshold && abs(gy) < gyroThreshold && abs(gz) < gyroThreshold) {
  return;
  }


  imuLowPassFilter(ax, ay, az, gx, gy, gz, mx, my, mz); //LPF

  float rollAcc = atan2(ay_filtered, az_filtered) * 180 / PI;
  float pitchAcc = atan(-ax_filtered / sqrt(ay_filtered * ay_filtered + az_filtered * az_filtered)) * 180 / PI;

  static float rollGyro = 0;
  static float pitchGyro = 0;
  static float yawGyro = 0;

  rollGyro += gx_filtered * dt;
  pitchGyro += gy_filtered * dt;
  yawGyro += gz_filtered *dt;

  roll = alpha * (roll + gx_filtered * dt) + (1 - alpha) * rollAcc;
  pitch = alpha * (pitch + gy_filtered * dt) + (1 - alpha) * pitchAcc;

  float yawMag = atan2(my_filtered, mx_filtered) * 180 / PI;
  
  yaw = alpha * (yaw + gz_filtered * dt) + (1 - alpha) * yawMag;

  normalizeIMUData(roll, pitch, yaw);
}

void setup() {

  filter.begin(200);

  Serial.begin(115200);

  initFilter();

  unsigned long startTime = millis();

  while (!Serial) {
    if (millis() - startTime > 3000) {
      break;
    }
  }

  Serial.println("Serial communication initialized");

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    // while (1);
    delay(2000);
    Serial.println("IMU time out! Continue without IMU...");
  }
  else{
    Serial.println("IMU initialized");
  }

  Wire.begin();

  delay(1000);

  i2cScanner();

  if (!particleSensor.begin()) {
    Serial.println("MAX30102 not found!");
    // while (1);
    delay(2000);
    Serial.println("MAX30102 time out! Continue without MAX30102...");
  }
  else{
    Serial.println("MAX30102 initialized!");
  }

  particleSensor.setMode(MAX30102::MODE_SPO2); // SPO2 模式
  particleSensor.setADCRange(MAX30102::ADC_RANGE_16384NA); // ADC 范围
  particleSensor.setSamplingRate(MAX30102::SAMPLING_RATE_200SPS); // 采样率 100Hz
  particleSensor.setResolution(MAX30102::RESOLUTION_18BIT_4110US); // 分辨率 脉冲宽度
  particleSensor.setLedCurrent(MAX30102::LED_RED, 60); // 红光 LED 电流（0-255，对应 0-51mA，每步 0.2mA）
  particleSensor.setLedCurrent(MAX30102::LED_IR, 60); // 红外光 LED 电流

  hrSpO2Calculator.begin(particleSensor, 100.0);

  initBLE();
}

void loop() {
  pollBLE();
  BLE.poll();
  if (central && central.connected()) {
    readAndFuseIMUData();

    bool newData = hrSpO2Calculator.update();

    heartRate = hrSpO2Calculator.getHeartRate();
    spo2 = hrSpO2Calculator.getSpO2();

    updateBLECharacteristics(roll_normalized, pitch_normalized, yaw_normalized, heartRate, spo2);
    printSensorData();
  }
  delay(dt * 100);
}

void printSensorData() {

  // Serial.print("加速度计 (ax, ay, az): ");
  // Serial.print(ax, 6); Serial.print(", ");
  // Serial.print(ay, 6); Serial.print(", ");
  // Serial.println(az, 6);
  // Serial.println("---------------------------");

  // Serial.print("陀螺仪 (gx, gy, gz): ");
  // Serial.print(gx, 6); Serial.print(", ");
  // Serial.print(gy, 6); Serial.print(", ");
  // Serial.println(gz, 6);
  // Serial.println("---------------------------");

  // Serial.print("磁力计 (mx, my, mz): ");
  // Serial.print(mx, 6); Serial.print(", ");
  // Serial.print(my, 6); Serial.print(", ");
  // Serial.println(mz, 6);
  // Serial.println("---------------------------");

  Serial.print("Normalised Roll=");
  Serial.print(roll_normalized, 4);
  Serial.print(", Normalised Pitch=");
  Serial.println(pitch_normalized, 4);
  Serial.print(", Normalised Yaw=");
  Serial.println(yaw_normalized, 4);
  Serial.println("---------------------------");

   Serial.print("Heartbeat (bpm): ");
   Serial.println(heartRate, 2);
   Serial.print(", SpO2 (%): ");
   Serial.println(spo2, 2);

  Serial.println("---------------------------");
  // Serial.println("Copyright: Arcadia_Ebendie / Yike Zhang");
  // Serial.println("---------------------------");

}

// void printSensorData() {
//   Serial.print(roll_normalized, 4);
//   Serial.print(" ");
//   Serial.print(pitch_normalized, 4);
//   Serial.print(" ");
//   Serial.println(yaw_normalized, 4); 
// }


