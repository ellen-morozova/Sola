#pragma once

#include <cstdint>
#include "hardware/pio.h"
#include <vector>

enum class ColorMode
{
    GENERAL,
    MODE2,
    RAINBOW,
    FIRE,
    BREATHING,
    PINKTOPURPLE,
    ORANGETOBLUE,
    BLUETOGREEN,
    PINKTOBLUETOORANGE,
    SUN,
    STROBERRY,
    NIGHTSKY,
    LEMON,
};

class LedController
{
public:
    struct LedColor
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };
    void showTwoColorTransition(const LedColor& color1, const LedColor& color2);
    void showThreeColorTransition(const LedColor& color1, const LedColor& color2, const LedColor& color3);
    void showRandomPalette(const std::vector<LedColor>& palette);
    void showColorWave(const LedColor& color1, const LedColor& color2);
    void init(PIO pio, uint32_t pin, uint32_t ledCount, uint32_t type);
    void setMode(ColorMode mode);
    void setBrightness(float brightness);
    void setBrightnessFromServo(float angle);
    void update1();
    void update2();

private:
    static uint8_t lerp8(uint8_t a, uint8_t b, float t);
    uint32_t transitionStartTime_ = 0;
    uint32_t randomUpdateTime_ = 0;
    uint32_t type_ = 1;
    std::vector<LedColor> transitionColors_;
    std::vector<LedColor> randomColors_;
    PIO pio_;
    unsigned int sm_;
    unsigned int pin_;
    unsigned int ledCount_;
    ColorMode mode_ = ColorMode::GENERAL;
    float brightness_ = 1.0f;
    float targetBrightness_ = 1.0f; 
    uint32_t rainbowOffset_ = 0;
    uint32_t animationTimer_ = 0;
    bool policeState_ = false;
    float breathingPhase_ = 0.0f;

    void putPixel(uint32_t pixel);
    uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);
    void fill(uint8_t r, uint8_t g, uint8_t b);
    void showRainbow();
    void showFire();
    void showPolice();
    void showBreathing();
};