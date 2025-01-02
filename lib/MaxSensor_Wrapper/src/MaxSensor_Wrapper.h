#ifndef MAXSENSOR_WRAPPER_H
#define MAXSENSOR_WRAPPER_H

#include <Arduino.h>
#include "max30102.h" // Include the MAX30102 library
#include "algorithm_by_RF.h"

class Sensor_Manager {
private:
  uint8_t pin;
  uint32_t aun_ir_buffer[BUFFER_SIZE];    // Infrared LED sensor data
  uint32_t aun_red_buffer[BUFFER_SIZE];   // Red LED sensor data
  int32_t true_heart_rate;
  int32_t finger_threshold;
  int dummy_heart_rate;
  bool data_ramping;
  bool heart_rate_valid;

public:
  Sensor_Manager(uint8_t pin, int32_t finger_threshold);
  void init();
  void setDataRamping(bool ramping);
  void reset();
  bool isFingerDetected();
  void update();
  int getHeartRate();
};

#endif // MAXSENSOR_WRAPPER_H