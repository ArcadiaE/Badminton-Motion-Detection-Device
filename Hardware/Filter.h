#ifndef FILTER_H
#define FILTER_H

extern const float imuFilterAlpha;

void initFilter();
void imuLowPassFilter(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz);
void normalizeIMUData(float roll, float pitch, float yaw);
float calculateAverage(float *buffer, int size);
// bool isValidIMUData(float ax, float ay, float az, float gx, float gy, float gz);
bool isValidIMUData(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz);

extern float ax_filtered, ay_filtered, az_filtered;
extern float gx_filtered, gy_filtered, gz_filtered;
extern float mx_filtered, my_filtered, mz_filtered;
extern float ax_normalized, ay_normalized, az_normalized;
extern float gx_normalized, gy_normalized, gz_normalized;
extern float mx_normalized, my_normalized, mz_normalized;
extern float roll_normalized, pitch_normalized, yaw_normalized;

#endif
