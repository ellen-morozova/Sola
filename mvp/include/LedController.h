#pragma once

#include <cstdint>

#include "hardware/pio.h"

enum class ColorMode
{
    RED,
    GREEN,
    BLUE,
    RAINBOW,
    FIRE,
    POLICE,
    BREATHING
};

class LedController
{
public:

    void init(PIO pio, uint32_t pin, uint32_t ledCount);

    void setMode(ColorMode mode);

    void setBrightness(float brightness);

    void update();

private:

    PIO pio_;
    unsigned sm_;

    unsigned pin_;

    unsigned ledCount_;

    ColorMode mode_ = ColorMode::RED;

    float brightness_ = 1.0f;

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