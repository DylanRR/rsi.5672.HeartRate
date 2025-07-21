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
  //communicationType = false; //WARNING: THIS IS A TEMP FIX FOR A BOARD WITH BAD JUMPERS
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

  Wire.setWireTimeout(1000 /* us */, true /* reset_on_timeout */);

  debugPrint("Setup complete!");
}

void setup_sensor(){
  debugPrint("Starting sensor initialization with diagnostic mode...");
  
  int result = bioHub.begin();
  if (!result) {
    debugPrint("Sensor found!");
  } else {
    char errorMsg[50];
    sprintf(errorMsg, "Could not communicate with sensor! Error: %d", result);
    debugPrint(errorMsg);
    return; // Exit if we can't communicate
  }

  debugPrint("Configuring Sensor - trying conservative MODE_ONE first...."); 
  
  // Start with MODE_ONE for reliability, then try MODE_TWO if needed
  int error = bioHub.configBpm(MODE_ONE);
  if(!error){
    debugPrint("Sensor configured with MODE_ONE (reliable mode).");
  }
  else {
    char errorMsg[50];
    sprintf(errorMsg, "MODE_ONE failed with error: %d", error);
    debugPrint(errorMsg);
    
    // Try MODE_TWO as fallback
    debugPrint("Trying MODE_TWO fallback...");
    error = bioHub.configBpm(MODE_TWO);
    if(!error) {
      debugPrint("Fallback configuration (MODE_TWO) successful.");
    } else {
      sprintf(errorMsg, "Both modes failed! Error: %d", error);
      debugPrint(errorMsg);
      return; // Exit if both modes fail
    }
  }

  // More conservative I2C timeout for stability
  Wire.setWireTimeout(1000 /* us */, true /* reset_on_timeout */);
  
  // Extended sensor testing to verify it's working
  debugPrint("Extended sensor diagnostic test...");
  for(int i = 0; i < 10; i++) {
    bioData testRead = bioHub.readBpm();
    char testMsg[150];
    sprintf(testMsg, "Diagnostic %d - Status: %d, HR: %d, Conf: %d, IR: %d, Red: %d", 
            i+1, testRead.status, testRead.heartRate, testRead.confidence, 
            testRead.irLed, testRead.redLed);
    debugPrint(testMsg);
    
    // Check if we're getting any response at all
    if (testRead.status != 0 || testRead.heartRate != 0 || testRead.confidence != 0 || 
        testRead.irLed != 0 || testRead.redLed != 0) {
      debugPrint("GOOD: Sensor is responding with data!");
      break;
    }
    
    delay(200); // Slower testing for stability
  }
  
  // Longer warmup time for better stability
  debugPrint("Extended sensor warmup (this is important for reliability)...");
  delay(4000); // Back to longer warmup
  
  // Final connectivity test
  debugPrint("Final connectivity test...");
  for(int i = 0; i < 5; i++) {
    bioData finalTest = bioHub.readBpm();
    char finalMsg[100];
    sprintf(finalMsg, "Final test %d - Status: %d, HR: %d, Confidence: %d", 
            i+1, finalTest.status, finalTest.heartRate, finalTest.confidence);
    debugPrint(finalMsg);
    delay(300);
  }
  
  debugPrint("Sensor initialization complete. Place finger firmly on sensor.");
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
      // Reduced delay for even faster sensor polling
      delayMicroseconds(50); // Ultra-short delay - about 0.05ms for faster response
    }
  }
  else {
    while (true) {
      SC_MAIN_LOOP();
      delayMicroseconds(50); // Same optimization for serial communication
    }
  }
}

void DC_MAIN_LOOP() {
  static int consecutiveValidStatusCount = 0; // Counter for consecutive 1, 2, or 3
  static int consecutiveZeroStatusCount = 0; // Counter for consecutive 0
  static bool fingerDetected = false;
  static unsigned long lastDebugPrint = 0;

  body = bioHub.readBpm();
  
  // Debug: Print raw sensor data every 2000ms (less frequent for cleaner output)
  if (millis() - lastDebugPrint > 2000) {
    char debugMsg[200];
    sprintf(debugMsg, "STABLE SENSOR - Status: %d, HR: %d, Conf: %d%%, Valid: %s", 
            body.status, body.heartRate, body.confidence,
            (body.status >= 1) ? "YES" : "NO");
    debugPrint(debugMsg);
    
    // Simplified feedback
    if (body.status == 0) {
      debugPrint("Place finger on sensor");
    } else if (body.status == 1) {
      debugPrint("Initializing measurement...");
    } else if (body.status >= 2) {
      debugPrint("Measuring heart rate - keep finger steady");
    }
    lastDebugPrint = millis();
  }

  // More stable detection logic
  if (body.status == 1 || body.status == 2 || body.status == 3) {
    consecutiveValidStatusCount++;
    consecutiveZeroStatusCount = 0; // Reset the zero counter

    // Require 2 consecutive readings for faster detection (reduced from 3)
    if (consecutiveValidStatusCount >= 2 && !fingerDetected) {
      fingerDetected = true;
      debugPrint("FINGER DETECTED - Starting heart rate measurement (FAST MODE)");
      // Immediately show finger detected state
      if (screenManager.getActiveState() == 0) {
        screenManager.setState(1);
        digitalWrite(FINGER_DETECTED_LED, HIGH);
      }
    }

    // Process heart rate data if finger is detected
    if (fingerDetected) {
      DC_RUN_STATE(&body);
    }
  }
  // Check if the status is 0 (no finger detected)
  else if (body.status == 0) {
    consecutiveZeroStatusCount++;
    consecutiveValidStatusCount = 0; // Reset the valid status counter

    // Require 10 consecutive zeros to avoid false removal (increased from 3)
    if (consecutiveZeroStatusCount >= 10 && fingerDetected) {
      fingerDetected = false;
      debugPrint("FINGER REMOVED - Thank you for using the heart rate monitor!");
      DC_IDLE_STATE();
    }
  }
}

void DC_RUN_STATE(bioData* body){
  static unsigned long lastHeartRateUpdate = 0;
  static int lastHeartRate = 0;
  static unsigned long lastStatusPrint = 0;
  static int stableReadingCount = 0;
  
  // Print status information every 2000ms for cleaner output
  if (millis() - lastStatusPrint > 2000) {
    char statusMsg[120];
    sprintf(statusMsg, "MEASURING - Status: %d, HR: %d, Conf: %d%%, Stable readings: %d", 
            body->status, body->heartRate, body->confidence, stableReadingCount);
    debugPrint(statusMsg);
    lastStatusPrint = millis();
  }
  
  // Focus on stable, high-confidence readings
  if (body->status >= 2) { // Status 2 or 3 (object/finger detected)
    
    // Faster confidence threshold for museum use (reduced from 50% to 40%)
    if (body->confidence >= 40 && body->heartRate > 40 && body->heartRate < 150) {
      digitalWrite(HEART_RATE_DETECTED_LED, HIGH);
      stableReadingCount++;
      
      // Only update display if heart rate has changed significantly or it's been a while
      int hrDifference = abs(body->heartRate - lastHeartRate);
      if (lastHeartRate == 0 || hrDifference >= 1 || (millis() - lastHeartRateUpdate > 3000)) {
        
        char hrMsg[100];
        sprintf(hrMsg, "HEART RATE: %d BPM (Confidence: %d%%, Count: %d) FAST", 
                body->heartRate, body->confidence, stableReadingCount);
        debugPrint(hrMsg);
        lastHeartRate = body->heartRate;
        lastHeartRateUpdate = millis();
        
        // Update display
        if (screenManager.getActiveState() == 1 || screenManager.getActiveState() == 2) {
          screenManager.setState(2, body->heartRate);
        }
      }
    }
    // Medium confidence readings - accept but don't make a big deal
    else if (body->confidence >= 30 && body->heartRate > 30 && body->heartRate < 180) {
      digitalWrite(HEART_RATE_DETECTED_LED, HIGH);
      
      // Only update if we haven't had a good reading in a while
      if (millis() - lastHeartRateUpdate > 5000) {
        debugPrint("Moderate confidence reading accepted for quick feedback");
        
        if (screenManager.getActiveState() == 1 || screenManager.getActiveState() == 2) {
          screenManager.setState(2, body->heartRate);
          lastHeartRateUpdate = millis();
        }
        lastHeartRate = body->heartRate;
      }
    }
  }
  // Status 1 means sensor is working but not quite ready
  else if (body->status == 1) {
    // Just keep the LED on to show we're working
    digitalWrite(HEART_RATE_DETECTED_LED, LOW); // Turn off heart rate LED
    // Keep finger detected LED on though
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
  static unsigned long lastStatusPrint = 0;
  static int lastHeartRate = 0;
  static unsigned long lastDebugPrint = 0;
  static int heartRateBuffer[3] = {0, 0, 0}; // Smoothing buffer
  static int bufferIndex = 0;
  static int validReadingsCount = 0;
  
  body = bioHub.readBpm();
  
  // Debug: Print raw sensor data every 1000ms
  if (millis() - lastDebugPrint > 1000) {
    char debugMsg[150];
    sprintf(debugMsg, "SC MUSEUM MODE - Status: %d, HR: %d, Conf: %d%%, Valid: %s", 
            body.status, body.heartRate, body.confidence, 
            (body.status >= 2) ? "YES" : "NO");
    debugPrint(debugMsg);
    lastDebugPrint = millis();
  }
  
  // Print status information every 750ms for debugging
  if (millis() - lastStatusPrint > 750) {
    char statusMsg[100];
    sprintf(statusMsg, "SC Processing - Status: %d, HR: %d, Conf: %d%%", 
            body.status, body.heartRate, body.confidence);
    debugPrint(statusMsg);
    lastStatusPrint = millis();
  }
  
  // Accept both object detected (2) and finger detected (3) for museum environment
  if (body.status == 2 || body.status == 3) {
    if (!SC_transmitting) {
      digitalWrite(FINGER_DETECTED_LED, HIGH);
      Serial.write(STX);
      SC_transmitting = true;
      debugPrint("SC: FINGER DETECTED - Starting transmission (museum mode)");
    }

    // Very low thresholds for children - prioritize speed over perfect accuracy
    if (body.heartRate > 40 && body.heartRate < 180 && body.confidence >= 3) {  // Even lower thresholds
      digitalWrite(HEART_RATE_DETECTED_LED, HIGH);
      validReadingsCount++;
      
      // Smooth the readings for better accuracy
      heartRateBuffer[bufferIndex] = body.heartRate;
      bufferIndex = (bufferIndex + 1) % 3;
      
      int smoothedHR;
      if (validReadingsCount >= 2) {
        smoothedHR = (heartRateBuffer[0] + heartRateBuffer[1] + heartRateBuffer[2]) / 3;
      } else {
        smoothedHR = body.heartRate; // Use raw reading for first few
      }
      
      // Transmit if heart rate is reasonable or hasn't changed dramatically
      int hrDifference = abs(smoothedHR - lastHeartRate);
      if (lastHeartRate == 0 || hrDifference <= 12) { // Allow 12 BPM variance for moving children
        
        char hrMsg[150];
        sprintf(hrMsg, "SC: TRANSMITTING HR: %d BPM (Conf: %d%%, Raw: %d, Smooth: %d)", 
                body.heartRate, body.confidence, body.heartRate, smoothedHR);
        debugPrint(hrMsg);
        lastHeartRate = smoothedHR;
        
        Serial.write(int32ToByte(smoothedHR));
        Serial.write(RS);
      }
      else {
        // Large change - still transmit raw but note it
        debugPrint("SC: Large HR change detected - child may be moving");
        Serial.write(int32ToByte(body.heartRate));
        Serial.write(RS);
        lastHeartRate = body.heartRate;
      }
    }
    // Transmit even marginal readings after a delay for impatient children
    else if (body.heartRate > 35 && body.heartRate < 200 && body.confidence >= 1) {
      if (validReadingsCount == 0 && millis() - lastDebugPrint > 4000) {
        debugPrint("SC: Transmitting low-confidence reading for quick feedback");
        Serial.write(int32ToByte(body.heartRate));
        Serial.write(RS);
        lastHeartRate = body.heartRate;
        validReadingsCount = 1;
      }
    }
  }
  // Status 1 (not ready) - don't immediately disconnect, sensor may be calculating
  else if (body.status == 1 && SC_transmitting) {
    // Keep transmitting - just not ready yet
    if (millis() - lastDebugPrint > 2000) {
      debugPrint("SC: Sensor calculating, maintaining connection...");
    }
  }
  // Only disconnect on actual finger removal (status 0)
  else if (body.status == 0) {
    if (SC_transmitting) {
      digitalWrite(FINGER_DETECTED_LED, LOW);
      digitalWrite(HEART_RATE_DETECTED_LED, LOW);
      Serial.write(ETX);
      SC_transmitting = false;
      debugPrint("SC: FINGER REMOVED - Thank you!");
      lastHeartRate = 0; // Reset for next session
      validReadingsCount = 0;
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