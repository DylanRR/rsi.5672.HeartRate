#include <Arduino.h>
#include <SPI.h>
#include "static.h"
#include "MaxSensor_Wrapper.h"
#include "OLED_Wrapper.h"
// Create the Screen_State_MANAGER object
Screen_State_MANAGER screenManager(OLED_CLK, OLED_MOSI, OLED_CS, OLED_DC, OLED_RESET);
Sensor_Manager sensorManager(MAX30102_INT, FINGER_THRESHOLD);

// Function Declarations
void RUN_STATE();
void IDLE_STATE();
void debugPrint(String location, String message = "");

void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup...");
  sensorManager.init();
  sensorManager.setDataRamping(true);
  Serial.println("MAX30102 sensor initialized...");
  screenManager.init();
  Serial.println("Display initialized...");
  screenManager.init();
  screenManager.setState(0);
  Serial.println("Screen manager initialized and state set...");
  Serial.println("Setup complete...");
}

void loop() {
  if (sensorManager.isFingerDetected()) {
    RUN_STATE();
  }
  else {
    IDLE_STATE();
  }
}

void RUN_STATE(){
  if (screenManager.getActiveState() == 0) {
      screenManager.setState(1);
  }

  int32_t temp_HR;
  temp_HR = sensorManager.getHeartRate();
  sensorManager.update();
  int32_t new_HR;
  new_HR = sensorManager.getHeartRate();

  if (temp_HR != new_HR) {
    if (screenManager.getActiveState() == 1 || screenManager.getActiveState() == 2) {
      screenManager.setState(2, new_HR);
    }
  }
}

void IDLE_STATE(){
  if (screenManager.getActiveState() != 0) {
    screenManager.setState(0);
    sensorManager.reset();
  }
}



void debugPrint(String message, String location = "") {
  if (DEBUG) {
    Serial.println(location + ": " + message);
  }
}
