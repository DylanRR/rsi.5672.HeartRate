
#define DEBUG true

// Pin definitions for the OLED display
#define OLED_CLK   52  // SCLK
#define OLED_MOSI  51  // SDIN (MOSI)\\ser
#define OLED_CS    53  // /CS
#define OLED_DC    49  // D/C
#define OLED_RESET 48  // /RES


// Global variables for serial communication
bool transmitting = false;        // Flag to indicate if the device is currently transmitting data over serial
unsigned long timer = 0;          // Timer used for sending SYNC bytes
#define SYNC_TIME 500             // Time to wait between sending SYNC bytes
#define SYN 0x16                  // Hexadecimal code for ASCII Synchronous Idle
#define STX 0x02                  // Hexadecimal code for ASCII Start of Text
#define ETX 0x03                  // Hexadecimal code for ASCII End of Text
#define RS 0x1E                   // Hexadecimal code for ASCII Record Separator
char ACTIVE_CHARS[4] = {SYN, STX, ETX, RS}; // Array of active characters for debugging purposes
bool communicationType; // Variable to store the type of communication (true for display, false for serial) 

// Global Pin definitions
#define DISPLAY_COMMUNICATION_PIN 7
#define SERIAL_COMMUNICATION_PIN 6
#define FINGER_DETECTED_LED 22
#define HEART_RATE_DETECTED_LED 24
#define SERIAL_OUTPUT_LED 26
#define DISPLAY_OUTPUT_LED 28


// No other Address options.
#define DEF_ADDR 0x55
// Reset pin, MFIO pin
const int resPin = 4;
const int mfioPin = 5;
