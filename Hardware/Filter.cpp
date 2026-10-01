#include "Filter.h"
#include <math.h>  

// Filter Coefficient
const float imuFilterAlpha = 0.1;

// Filerred Data
float ax_filtered = 0, ay_filtered = 0, az_filtered = 0;
float gx_filtered = 0, gy_filtered = 0, gz_filtered = 0;
float mx_filtered = 0, my_filtered = 0, mz_filtered = 0;

// Normalised Data
float ax_normalized = 0, ay_normalized = 0, az_normalized = 0;
float gx_normalized = 0, gy_normalized = 0, gz_normalized = 0;
float mx_normalized = 0, my_normalized = 0, mz_normalized = 0;
float roll_normalized = 0, pitch_normalized = 0, yaw_normalized = 0;

void initFilter() {
  ax_filtered = ay_filtered = az_filtered = 0;
  gx_filtered = gy_filtered = gz_filtered = 0;
  mx_filtered = my_filtered = mz_filtered = 0;
}

// LPF
// void imuLowPassFilter(float ax, float ay, float az, float gx, float gy, float gz) {
//   // 加速度计滤波
//   ax_filtered = imuFilterAlpha * ax + (1 - imuFilterAlpha) * ax_filtered;
//   ay_filtered = imuFilterAlpha * ay + (1 - imuFilterAlpha) * ay_filtered;
//   az_filtered = imuFilterAlpha * az + (1 - imuFilterAlpha) * az_filtered;

//   // 陀螺仪滤波
//   gx_filtered = imuFilterAlpha * gx + (1 - imuFilterAlpha) * gx_filtered;
//   gy_filtered = imuFilterAlpha * gy + (1 - imuFilterAlpha) * gy_filtered;
//   gz_filtered = imuFilterAlpha * gz + (1 - imuFilterAlpha) * gz_filtered;
// }

void imuLowPassFilter(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz) {
  ax_filtered = imuFilterAlpha * ax + (1 - imuFilterAlpha) * ax_filtered;
  ay_filtered = imuFilterAlpha * ay + (1 - imuFilterAlpha) * ay_filtered;
  az_filtered = imuFilterAlpha * az + (1 - imuFilterAlpha) * az_filtered;

  gx_filtered = imuFilterAlpha * gx + (1 - imuFilterAlpha) * gx_filtered;
  gy_filtered = imuFilterAlpha * gy + (1 - imuFilterAlpha) * gy_filtered;
  gz_filtered = imuFilterAlpha * gz + (1 - imuFilterAlpha) * gz_filtered;

  mx_filtered = imuFilterAlpha * mx + (1 - imuFilterAlpha) * mx_filtered;
  my_filtered = imuFilterAlpha * my + (1 - imuFilterAlpha) * my_filtered;
  mz_filtered = imuFilterAlpha * mz + (1 - imuFilterAlpha) * mz_filtered;
}


// Normalisation
void normalizeIMUData(float roll, float pitch, float yaw) {
  //Datasheet defined
  ax_normalized = (ax_filtered + 4.0) / 8.0;
  ay_normalized = (ay_filtered + 4.0) / 8.0;
  az_normalized = (az_filtered + 4.0) / 8.0;

  gx_normalized = (gx_filtered + 2000.0) / 4000.0;
  gy_normalized = (gy_filtered + 2000.0) / 4000.0;
  gz_normalized = (gz_filtered + 2000.0) / 4000.0;

  mx_normalized = (mx_filtered + 1300.0) / 2600.0;
  my_normalized = (my_filtered + 1300.0) / 2600.0;
  mz_normalized = (mz_filtered + 2500.0) / 5000.0;

  roll_normalized = (roll + 180.0) / 360.0;
  pitch_normalized = (pitch + 180.0) / 360.0;
  yaw_normalized = (yaw + 180.0) / 360.0;
}

float calculateAverage(float *buffer, int size) {
  float sum = 0;
  for (int i = 0; i < size; i++) {
    sum += buffer[i];
  }
  return sum / size;
}

bool isValidIMUData(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz) {
  const float MAG_XY_MAX = 1300.0; 
  const float MAG_Z_MAX = 2500.0; 
  
  if (abs(mx) > MAG_XY_MAX || abs(my) > MAG_XY_MAX || abs(mz) > MAG_Z_MAX) {
    return false;
  }
  if (abs(ax) > 16.0 || abs(ay) > 16.0 || abs(az) > 16.0) {
    return false;
  }
  if (abs(gx) > 2000.0 || abs(gy) > 2000.0 || abs(gz) > 2000.0) {
    return false;
  }
  return true;
}
