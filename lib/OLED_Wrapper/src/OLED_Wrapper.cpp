#include "OLED_Wrapper.h"

Screen_State_MANAGER::Screen_State_MANAGER(uint8_t clk, uint8_t mosi, uint8_t cs, uint8_t dc, uint8_t reset)
  :u8g2(U8G2_R0, /* rotation */
    clk, /* clock */
    mosi, /* data */
    cs, /* cs */
    dc, /* dc */
    reset) {
  u8g2.begin();
}

void Screen_State_MANAGER::init() {
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    this->idle.Reset();
    this->reading.Reset();
    this->result.Reset();
}

void Screen_State_MANAGER::Screen_State_IDLE::Reset() {
    screenInIdleState = false;
}

void Screen_State_MANAGER::Screen_State_READING::Reset() {
    screenInReadingState = false;
}

// Implement the Reset function for Screen_State_RESULT
void Screen_State_MANAGER::Screen_State_RESULT::Reset() {
    screenInResultState = false;
    heartRate = 0;
    timer = 0;
}

void Screen_State_MANAGER::setState(int state, int32_t heart_rate) {
    // Reset all states
    this->idle.Reset();
    this->reading.Reset();
    this->result.Reset();

    switch (state) {
        case 0:
            this->idle.display(u8g2);
            break;
        case 1:
            this->reading.display(u8g2);
            break;
        case 2:
            if (heart_rate == 0) {
                Serial.println("setState, Heart rate is 0.");
            }
            this->result.display(u8g2, heart_rate);
            break;
        default:
            Serial.println("setState, Invalid state.");
    }
}

int Screen_State_MANAGER::getActiveState() {
    int activeState = -1;
    int activeCount = 0;

    if (this->idle.screenInIdleState) {
        activeState = 0;
        activeCount++;
    }
    if (this->reading.screenInReadingState) {
        activeState = 1;
        activeCount++;
    }
    if (this->result.screenInResultState) {
        activeState = 2;
        activeCount++;
    }

    if (activeCount != 1) {
        Serial.println("getActiveState, Multiple states are active.");
        return -1;
    }

    return activeState;
}

void Screen_State_MANAGER::update() {
    if (this->result.screenInResultState) {
        this->result.display(u8g2, 0);
    }
}

// Display functions moved to the cpp file

void Screen_State_MANAGER::Screen_State_IDLE::display(U8G2_SSD1322_NHD_256X64_F_4W_SW_SPI& u8g2) {
    if (screenInIdleState) {
        return;
    }
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB14_tr);
    const char* message = "Place finger on sensor";
    int16_t x = (u8g2.getDisplayWidth() - u8g2.getStrWidth(message)) / 2;
    int16_t y = (u8g2.getDisplayHeight() + u8g2.getMaxCharHeight()) / 2;
    u8g2.drawStr(x, y, message);
    u8g2.sendBuffer();
    this->screenInIdleState = true;
}

void Screen_State_MANAGER::Screen_State_READING::display(U8G2_SSD1322_NHD_256X64_F_4W_SW_SPI& u8g2) {
    if (screenInReadingState) {
        return;
    }
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB14_tr);
    const char* message = "Reading Heart Rate...";
    int16_t x = (u8g2.getDisplayWidth() - u8g2.getStrWidth(message)) / 2;
    int16_t y = (u8g2.getDisplayHeight() + u8g2.getMaxCharHeight()) / 2;
    u8g2.drawStr(x, y, message);
    u8g2.sendBuffer();
    this->screenInReadingState = true;
}

// Implement the display function for Screen_State_RESULT with one parameter (does nothing)
void Screen_State_MANAGER::Screen_State_RESULT::display(U8G2_SSD1322_NHD_256X64_F_4W_SW_SPI& u8g2) {
    // Do nothing
}

void Screen_State_MANAGER::Screen_State_RESULT::display(U8G2_SSD1322_NHD_256X64_F_4W_SW_SPI& u8g2, int32_t heart_rate) {
    this->screenInResultState = true;
    if (!__reDrawState(heart_rate)) {
        return;
    }

    u8g2.clearBuffer(); // Clear display buffer

    // Draw heart rate
    char buffer[4];
    sprintf(buffer, "%03d", heartRate);
    u8g2.setFont(u8g2_font_inb53_mn);
    int16_t x = (u8g2.getDisplayWidth() - u8g2.getStrWidth(buffer)) / 2;
    int16_t y = (u8g2.getDisplayHeight() + u8g2.getMaxCharHeight()) / 2 - 10;
    u8g2.drawStr(x, y, buffer);

    // Draw "bpm" text
    u8g2.setFont(u8g2_font_ncenB14_tr);
    int16_t bpm_x = x + u8g2.getStrWidth(buffer) + 5; // Adjust the x-coordinate for "bpm"
    int16_t bpm_y = y + (u8g2.getMaxCharHeight() / 2); // Adjust the y-coordinate for "bpm"
    u8g2.drawStr(bpm_x, bpm_y, "bpm");

    u8g2.sendBuffer();  // Send buffer to display
}

bool Screen_State_MANAGER::Screen_State_RESULT::__reDrawState(int32_t heart_rate) {
    if (heart_rate != this->heartRate && heart_rate != 0) {
        this->heartRate = static_cast<int>(heart_rate);
        return true;
    }
    return false;
}