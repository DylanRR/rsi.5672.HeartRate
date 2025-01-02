
#define DEBUG false

// Pin definitions for the OLED display
#define OLED_CLK   52  // SCLK
#define OLED_MOSI  51  // SDIN (MOSI)\\ser
#define OLED_CS    53  // /CS
#define OLED_DC    49  // D/C
#define OLED_RESET 48  // /RES

// Pin definitions for the MAX30102 sensor
#define MAX30102_INT 3

// Global Timer variable
unsigned long timer = 0;

// Global variables for the MAX30102 sensor
#define FINGER_THRESHOLD 60000
bool transmitting = false;
