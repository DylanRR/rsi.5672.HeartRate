#include <Arduino.h>
#include <SPI.h>
#include <SparkFun_Bio_Sensor_Hub_Library.h>
#include <OLED_Wrapper.h>
#include "static.h"
// Create the Screen_State_MANAGER object
Screen_State_MANAGER screenManager(OLED_CLK, OLED_MOSI, OLED_CS, OLED_DC, OLED_RESET);

// Takes address, reset pin, and MFIO pin.
SparkFun_Bio_Sensor_Hub bioHub(resPin, mfioPin); 
bioData body;

// Function Declarations
void RUN_STATE();
void IDLE_STATE();
bool setup_determineCommunicationType();
void DC_MAIN_LOOP();
void DC_RUN_STATE(bioData* body);
void DC_IDLE_STATE();
void SC_MAIN_LOOP();
void setup_sensor();
byte int32ToByte(int32_t value);

void debugPrint(char* message);
void debugPrint(char* message, char* location);
String message_scrubber(const char* message);

void setup() {
  Serial.begin(115200);
  Wire.begin();
  debugPrint("Begining setup...");
  pinMode(SERIAL_OUTPUT_LED, OUTPUT);
  pinMode(DISPLAY_OUTPUT_LED, OUTPUT);
  communicationType = setup_determineCommunicationType();
  debugPrint("Communication setup complete...");
  pinMode(FINGER_DETECTED_LED, OUTPUT);
  pinMode(HEART_RATE_DETECTED_LED, OUTPUT);
  
  setup_sensor();
  debugPrint("Max Sensor initialized...");

  if (communicationType) {
    screenManager.init();
    debugPrint("Display initialized...");
    screenManager.setState(0);
    debugPrint("Display set to 0 state...");
    digitalWrite(DISPLAY_OUTPUT_LED, HIGH);
  }
  else {
    digitalWrite(SERIAL_OUTPUT_LED, HIGH);
  }
  debugPrint("Setup complete!");
}

void setup_sensor(){
  int result = bioHub.begin();
  if (!result)
    debugPrint("Sensor found!");
  else
  debugPrint("Could not communicate with the sensor!!!");

  debugPrint("Configuring Sensor...."); 
  int error = bioHub.configBpm(MODE_ONE); // Configuring just the BPM settings. 
  if(!error){
    debugPrint("Sensor configured.");
  }
  else {
    debugPrint("Error configuring sensor.");
  }
  delay(2000); // Wait for the sensor to stabilize.
}

bool setup_determineCommunicationType() {
  debugPrint("Determining communication type...");
  pinMode(DISPLAY_COMMUNICATION_PIN, INPUT);
  pinMode(SERIAL_COMMUNICATION_PIN, INPUT);
  bool DCP = digitalRead(DISPLAY_COMMUNICATION_PIN);
  bool SCP = digitalRead(SERIAL_COMMUNICATION_PIN);
  if (DCP && SCP) {
    debugPrint("ERROR: DCP and SCP are both HIGH", "setup_determineCommunicationType");
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
    debugPrint("ERROR: DCP and SCP are both LOW", "setup_determineCommunicationType");
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
    debugPrint("Communication type: Display");
    return true;
  }
  else {
    debugPrint("Communication type: Serial");
    debugPrint("Now scrubbing debug messages...");
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
  body = bioHub.readBpm();
  if (body.status == 1) {
    DC_RUN_STATE(&body);
  }
  else {
    DC_IDLE_STATE();
  }
}

void DC_RUN_STATE(bioData* body){
  if (screenManager.getActiveState() == 0) {
      screenManager.setState(1);
      digitalWrite(FINGER_DETECTED_LED, HIGH);
  }

  if (body->status == 3 && body->heartRate > 0 && body->confidence > 50) {  //TODO: Add a confidence var to the static file
    digitalWrite(HEART_RATE_DETECTED_LED, HIGH);
    if (screenManager.getActiveState() == 1 || screenManager.getActiveState() == 2) {
      screenManager.setState(2, body->heartRate);
    }
  }
}

void DC_IDLE_STATE(){
  if (screenManager.getActiveState() != 0) {
    digitalWrite(FINGER_DETECTED_LED, LOW);
    digitalWrite(HEART_RATE_DETECTED_LED, LOW);
    screenManager.setState(0);
  }
}

bool SC_transmitting = false;
void SC_MAIN_LOOP() {
  body = bioHub.readBpm();
  if (body.status == 1 || body.status == 2 || body.status == 3) {
    if (!SC_transmitting) {
      digitalWrite(FINGER_DETECTED_LED, HIGH);
      Serial.write(STX);
      SC_transmitting = true;
    }

    if (body.status == 3 && body.heartRate > 30 && body.confidence > 50 ) {  //TODO: Add a confidence var to the static file
      digitalWrite(HEART_RATE_DETECTED_LED, HIGH);
      //Serial.println(String(body.heartRate));
      Serial.write(int32ToByte(body.heartRate));
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



void debugPrint(char* message, char* location) {
  if (DEBUG) {
    String temp = String(message) + " @ " + String(location); // Concatenate message and location with " @ " in between
    String scrubbed = message_scrubber(temp.c_str());         // Pass the concatenated string to message_scrubber
    Serial.print(scrubbed);                                   //New line is added by the scrubber
  }
}

void debugPrint(char* message) {
  if (DEBUG) {
    String scrubbed = message_scrubber(message);
    Serial.print(scrubbed); //New line is added by the scrubber
  }
}

String message_scrubber(const char* message) {
  if (communicationType) {
    return String(message) + "\n";
  }
  Serial.println("Scrubber");
  String result = "\n";
  for (int i = 0; i < strlen(message); i++) {
    bool isActiveChar = false;
    for (int j = 0; j < sizeof(ACTIVE_CHARS); j++) {
      if (message[i] == ACTIVE_CHARS[j]) {
        isActiveChar = true;
        break;
      }
    }
    if (!isActiveChar) {
      result += message[i];
    }
  }
  result += '\n';
  return result;
}