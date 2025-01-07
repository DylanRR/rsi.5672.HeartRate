#ifndef OLED_WRAPPER_H
#define OLED_WRAPPER_H

#include <Arduino.h>
#include <U8g2lib.h>

class Screen_State_MANAGER {
private:
    U8G2_SSD1322_NHD_256X64_F_4W_SW_SPI u8g2; // Declare u8g2 as a member

    class Screen_State {
    public:
        virtual void Reset() = 0;
        virtual void display(U8G2_SSD1322_NHD_256X64_F_4W_SW_SPI& u8g2) = 0; // Use reference
    };

    class Screen_State_IDLE : public Screen_State {
    public:
        bool screenInIdleState;

        void Reset() override;
        void display(U8G2_SSD1322_NHD_256X64_F_4W_SW_SPI& u8g2) override; // Use reference
    };

    class Screen_State_READING : public Screen_State {
    public:
        bool screenInReadingState;

        void Reset() override;
        void display(U8G2_SSD1322_NHD_256X64_F_4W_SW_SPI& u8g2) override; // Use reference
    };

    class Screen_State_RESULT : public Screen_State {
    public:
        bool screenInResultState;
        int heartRate;
        unsigned long timer;

        void Reset() override;
        void display(U8G2_SSD1322_NHD_256X64_F_4W_SW_SPI& u8g2) override; // Use reference
        void display(U8G2_SSD1322_NHD_256X64_F_4W_SW_SPI& u8g2, int32_t heart_rate); // Use reference
        bool __reDrawState(int32_t heart_rate);
    };

    Screen_State_IDLE idle;
    Screen_State_READING reading;
    Screen_State_RESULT result;

public:
    Screen_State_MANAGER(uint8_t clk, uint8_t mosi, uint8_t cs, uint8_t dc, uint8_t reset);
    void init();
    void setState(int state, int32_t heart_rate = 0);
    int getActiveState();
    void update();
};

#endif // OLED_WRAPPER_H