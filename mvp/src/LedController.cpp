#include "LedController.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <cstdio>

static inline void ws2812_program_init_manual(
    PIO pio,
    unsigned int sm,
    unsigned int offset,
    unsigned int pin,
    float freq,
    bool rgbw)
{
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);

    pio_sm_config c = ws2812_program_get_default_config(offset);
    sm_config_set_sideset_pins(&c, pin);
    sm_config_set_out_shift(&c, false, true, rgbw ? 32 : 24);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    int cycles_per_bit = ws2812_T1 + ws2812_T2 + ws2812_T3;
    float div = (float)clock_get_hz(clk_sys) / (freq * cycles_per_bit);
    sm_config_set_clkdiv(&c, div);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;
}

void LedController::init(PIO pio, uint32_t pin, uint32_t ledCount)
{
    pio_ = pio;
    pin_ = pin;
    ledCount_ = ledCount;

    uint32_t offset = pio_add_program(pio_, &ws2812_program);
    sm_ = pio_claim_unused_sm(pio_, true);

    ws2812_program_init_manual(pio_, sm_, offset, pin_, 800000.0f, false);

    brightness_ = 1.0f;
    targetBrightness_ = 1.0f;
    rainbowOffset_ = 0;
    animationTimer_ = 0;
    policeState_ = false;
    breathingPhase_ = 0.0f;

    for (uint32_t i = 0; i < ledCount_; ++i)
    {
        putPixel(0);
    }
    sleep_us(80);

    srand(time_us_32());
}

void LedController::setMode(ColorMode mode)
{
    mode_ = mode;
}

void LedController::setBrightness(float brightness)
{
    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > 1.0f) brightness = 1.0f;

    brightness_ = brightness;

    printf("Manual brightness = %.2f\n", brightness_);
}

void LedController::setBrightnessFromServo(float angle)
{
    float newBrightness = angle / 45.0f;

    if (newBrightness < 0.0f) newBrightness = 0.0f;
    if (newBrightness > 1.0f) newBrightness = 1.0f;

    brightness_ = newBrightness;

    printf("Servo angle = %.2f, Servo brightness = %.2f\n", angle, brightness_);
}

void LedController::putPixel(uint32_t pixel)
{
    pio_sm_put_blocking(pio_, sm_, pixel << 8u);
}

uint32_t LedController::rgb(uint8_t r, uint8_t g, uint8_t b)
{
    r = (uint8_t)((float)r * brightness_);
    g = (uint8_t)((float)g * brightness_);
    b = (uint8_t)((float)b * brightness_);

    return urgb_u32(r, g, b);
}

void LedController::fill(uint8_t r, uint8_t g, uint8_t b)
{
    printf("FILL: r=%d g=%d b=%d brightness=%.2f\n", r, g, b, brightness_);

    uint32_t color = rgb(r, g, b);

    for (uint32_t i = 0; i < ledCount_; i++)
    {
        putPixel(color);
    }
}

void LedController::showRainbow()
{
    for (uint32_t i = 0; i < ledCount_; i++)
    {
        uint8_t pos = (uint8_t)((i * 256 / ledCount_ + rainbowOffset_) & 0xFF);
        uint8_t r, g, b;

        if (pos < 85)
        {
            r = pos * 3;
            g = 255 - pos * 3;
            b = 0;
        }
        else if (pos < 170)
        {
            pos -= 85;
            r = 255 - pos * 3;
            g = 0;
            b = pos * 3;
        }
        else
        {
            pos -= 170;
            r = 0;
            g = pos * 3;
            b = 255 - pos * 3;
        }

        putPixel(rgb(r, g, b));
    }

    rainbowOffset_++;
}

void LedController::showFire()
{
    for (uint32_t i = 0; i < ledCount_; i++)
    {
        uint8_t r = 255;
        uint8_t g = 20 + rand() % 150;
        putPixel(rgb(r, g, 0));
    }
}

void LedController::showPolice()
{
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (now - animationTimer_ > 250)
    {
        animationTimer_ = now;
        policeState_ = !policeState_;
    }

    if (policeState_)
        fill(255, 0, 0);
    else
        fill(0, 0, 255);
}

void LedController::showBreathing()
{
    breathingPhase_ += 0.05f;
if (breathingPhase_ > 6.28f)
        breathingPhase_ = 0.0f;

    float k = (sinf(breathingPhase_) + 1.0f) * 0.5f;
    uint8_t value = (uint8_t)(255 * k);

    fill(value, value, value);
}

void LedController::update()
{
    printf("Mode: %d, Brightness: %.2f\n", (int)mode_, brightness_);

    switch (mode_)
    {
        case ColorMode::RED:
            fill(255, 0, 0);
            break;

        case ColorMode::GREEN:
            fill(0, 255, 0);
            break;

        case ColorMode::BLUE:
            fill(0, 0, 255);
            break;

        case ColorMode::RAINBOW:
            showRainbow();
            break;

        case ColorMode::FIRE:
            showFire();
            break;

        case ColorMode::POLICE:
            showPolice();
            break;

        case ColorMode::BREATHING:
            showBreathing();
            break;
    }
}