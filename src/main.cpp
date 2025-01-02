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
bool setup_determineCommunicationType();
void DC_MAIN_LOOP();
void DC_RUN_STATE();
void DC_IDLE_STATE();
void SC_MAIN_LOOP();
byte int32ToByte(int32_t value);

bool communicationType;

void setup() {
  Serial.begin(115200);
  pinMode(SERIAL_OUTPUT_LED, OUTPUT);
  pinMode(DISPLAY_OUTPUT_LED, OUTPUT);
  communicationType = setup_determineCommunicationType();
  pinMode(FINGER_DETECTED_LED, OUTPUT);
  pinMode(HEART_RATE_DETECTED_LED, OUTPUT);
  
  sensorManager.init();

  if (communicationType) {
    sensorManager.setDataRamping(true);
    screenManager.init();
    screenManager.setState(0);
    digitalWrite(DISPLAY_OUTPUT_LED, HIGH);
  }
  else {
    sensorManager.setDataRamping(false);
    digitalWrite(SERIAL_OUTPUT_LED, HIGH);
  }
}

bool setup_determineCommunicationType() {
  pinMode(DISPLAY_COMMUNICATION_PIN, INPUT);
  pinMode(SERIAL_COMMUNICATION_PIN, INPUT);
  bool DCP = digitalRead(DISPLAY_COMMUNICATION_PIN);
  bool SCP = digitalRead(SERIAL_COMMUNICATION_PIN);
  if (DCP && SCP) {
    while (true) {
      digitalWrite(DISPLAY_OUTPUT_LED, HIGH);
      digitalWrite(SERIAL_OUTPUT_LED, HIGH);
      delay(500);
      digitalWrite(DISPLAY_OUTPUT_LED, LOW);
      digitalWrite(SERIAL_OUTPUT_LED, LOW);
      delay(500);
    }
  }
  else if (!DCP && !SCP) {
    while (true) {
      digitalWrite(DISPLAY_OUTPUT_LED, HIGH);
      delay(250);
      digitalWrite(DISPLAY_OUTPUT_LED, LOW);
      delay(250);
      digitalWrite(SERIAL_OUTPUT_LED, HIGH);
      delay(250);
      digitalWrite(SERIAL_OUTPUT_LED, LOW);
      delay(250);
    }
  }
  else if (DCP) {
    return true;
  }
  else {
    return false;
  }
}

void loop() {
  if (communicationType) {
    while (true) {
      DC_MAIN_LOOP();
    }
  }
  else {
    while (true) {
      SC_MAIN_LOOP();
    }
  }
}

void DC_MAIN_LOOP() {
  if (sensorManager.isFingerDetected()) {
    DC_RUN_STATE();
  }
  else {
    DC_IDLE_STATE();
  }
}

void DC_RUN_STATE(){
  if (screenManager.getActiveState() == 0) {
      screenManager.setState(1);
      digitalWrite(FINGER_DETECTED_LED, HIGH);
  }

  int32_t temp_HR;
  temp_HR = sensorManager.getHeartRate();
  sensorManager.update();
  int32_t new_HR;
  new_HR = sensorManager.getHeartRate();

  if (temp_HR != new_HR) {
    digitalWrite(HEART_RATE_DETECTED_LED, HIGH);
    if (screenManager.getActiveState() == 1 || screenManager.getActiveState() == 2) {
      screenManager.setState(2, new_HR);
    }
  }
}

void DC_IDLE_STATE(){
  if (screenManager.getActiveState() != 0) {
    digitalWrite(FINGER_DETECTED_LED, LOW);
    digitalWrite(HEART_RATE_DETECTED_LED, LOW);
    screenManager.setState(0);
    sensorManager.reset();
  }
}

bool SC_transmitting = false;
void SC_MAIN_LOOP() {
  if (sensorManager.isFingerDetected()) {
    if (!SC_transmitting) {
      digitalWrite(FINGER_DETECTED_LED, HIGH);
      Serial.write(STX);
      SC_transmitting = true;
    }
    int32_t temp_HR;
    temp_HR = sensorManager.getHeartRate();
    sensorManager.update();
    int32_t new_HR;
    new_HR = sensorManager.getHeartRate();
    if (temp_HR != new_HR) {
      digitalWrite(HEART_RATE_DETECTED_LED, HIGH);
      Serial.write(int32ToByte(new_HR));
      Serial.write(RS);
    }
  }
  else {
    if (SC_transmitting) {
      digitalWrite(FINGER_DETECTED_LED, LOW);
      digitalWrite(HEART_RATE_DETECTED_LED, LOW);
      Serial.write(ETX);
      SC_transmitting = false;
    }
    if (!SC_transmitting && millis() - timer > SYNC_TIME) {
      Serial.write(SYN);
      timer = millis();
    }
  }
}

byte int32ToByte(int32_t value) {
  if (value < 0) {
    return 0; // Round to minimum value of a byte
  } else if (value > 255) {
    return 255; // Round to maximum value of a byte
  } else {
    return (byte)value; // Cast to byte
  }
}


void debugPrint(String message, String location = "") {
  if (DEBUG) {
    Serial.println(location + ": " + message);
  }
}
