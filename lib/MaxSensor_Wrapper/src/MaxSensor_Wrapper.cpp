#include "MaxSensor_Wrapper.h"

// Constructor
Sensor_Manager::Sensor_Manager(uint8_t pin, int32_t finger_threshold) {
  this->pin = pin;
  this->finger_threshold = finger_threshold;
}

// Initialize the sensor manager
void Sensor_Manager::init() {
  this->true_heart_rate = 0;
  this->dummy_heart_rate = 0;
  this->data_ramping = false;
  this->heart_rate_valid = false;
  memset(this->aun_ir_buffer, 0, sizeof(this->aun_ir_buffer));
  memset(this->aun_red_buffer, 0, sizeof(this->aun_red_buffer));
  memset(this->rolling_queue, 0, sizeof(this->rolling_queue));
  this->queue_index = 0;

  pinMode(this->pin, INPUT_PULLUP);
  maxim_max30102_init(); // Initialize the MAX30102
}

// Reset the sensor manager
void Sensor_Manager::reset() {
  this->true_heart_rate = 0;
  this->dummy_heart_rate = 0;
  this->heart_rate_valid = false;
  memset(this->aun_ir_buffer, 0, sizeof(this->aun_ir_buffer));
  memset(this->aun_red_buffer, 0, sizeof(this->aun_red_buffer));
  memset(this->rolling_queue, 0, sizeof(this->rolling_queue));
  this->queue_index = 0;
}

void Sensor_Manager::setDataRamping(bool ramping){
  this->data_ramping = ramping;
}

int Sensor_Manager::getHeartRate(){
  if (this->data_ramping){
    return static_cast<int>(this->dummy_heart_rate);
  }
  return static_cast<int>(this->true_heart_rate);
}

// Update the sensor manager
void Sensor_Manager::update() {
  int32_t n_heart_rate;             // Heart rate value
  int8_t ch_hr_valid;               // Indicator to show if the heart rate calculation is valid
  float n_spo2;                     // SpO₂ value (unused)
  int8_t ch_spo2_valid;             // Indicator to show if the SpO₂ calculation is valid (unused)
  float ratio;                      // Ratio (unused)
  float correl;

  for (int i = 0; i < BUFFER_SIZE; i++) {
    while (digitalRead(this->pin) == 1); // Wait until the interrupt pin asserts
    maxim_max30102_read_fifo(this->aun_red_buffer + i, this->aun_ir_buffer + i); // Read from MAX30102 FIFO
  }
  
  rf_heart_rate_and_oxygen_saturation(   // Look at trying out the maixim algorithm called maxim_heart_rate_and_oxygen_saturation
    aun_ir_buffer, BUFFER_SIZE, aun_red_buffer,
    &n_spo2, &ch_spo2_valid,      // Provide addresses for SpO₂ parameters
    &n_heart_rate, &ch_hr_valid,
    &ratio, &correl               // Provide addresses for ratio and correlation
  );
  

  if (ch_hr_valid) {
    Serial.println("Valid Heart Rate" + String(n_heart_rate));
    if (this->data_ramping && this->true_heart_rate == 0){
      this->dummy_heart_rate = n_heart_rate;  //NOTE: This may have to be constrained to a certain range
    }

    this->true_heart_rate =  n_heart_rate;
  }
  else {
    //Serial.println("Invalid Heart Rate");
    Serial.println("Invalid Heart Rate: " + String(n_heart_rate));
  }


  // Add the new heart rate to the rolling queue
  this->rolling_queue[this->queue_index] = n_heart_rate;
  this->queue_index = (this->queue_index + 1) % 3; // Update the index in a circular manner
  
  if (this->checkRollingQueue()){
    this->dummy_heart_rate = this->true_heart_rate;
  }

  if (this->true_heart_rate > this->dummy_heart_rate){
      this->dummy_heart_rate++;
  }
  else if (this->true_heart_rate < this->dummy_heart_rate){
    this->dummy_heart_rate--;
  }
  
}

bool Sensor_Manager::isFingerDetected(){
  maxim_max30102_read_fifo(this->aun_red_buffer, this->aun_ir_buffer);
  if (this->aun_red_buffer[0] > this->finger_threshold || this->aun_ir_buffer[0] > this->finger_threshold) {
    return true;
  }
  return false;
}

bool Sensor_Manager::checkRollingQueue() {
  return (this->rolling_queue[0] == this->rolling_queue[1] && 
    this->rolling_queue[1] == this->rolling_queue[2] && 
    this->rolling_queue[0] != -999);
}
