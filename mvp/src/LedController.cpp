#include "LedController.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"
#include <cstdlib>
#include <cmath>
#include <ctime>

static inline void ws2812_program_init(PIO pio, uint32_t sm, uint32_t offset, uint32_t pin, float freq, bool rgbw) {
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
    pio_sm_set_pins_with_mask(pio, sm, 0, 1u << pin);
    pio_sm_set_pindirs_with_mask(pio, sm, 1u << pin, 1u << pin);
    
    pio_sm_init(pio, sm, offset, NULL);
    
    float div = (float)clock_get_hz(clk_sys) / (freq * 8.0f);
    pio_sm_set_clkdiv(pio, sm, div);
    
    pio_sm_config c = ws2812_program_get_default_config(offset);
    sm_config_set_out_shift(&c, true, true, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_sideset_pins(&c, pin);
    
    pio_sm_set_config(pio, sm, &c);
    pio_sm_set_enabled(pio, sm, true);
}

static inline uint32_t urgb_u32(
    uint8_t r,
    uint8_t g,
    uint8_t b)
{
    return
        ((uint32_t)g << 16) |
        ((uint32_t)r << 8)  |
        b;
}

void LedController::init(PIO pio, uint32_t pin, uint32_t ledCount)
{
    pio_ = pio;
    pin_ = pin;
    ledCount_ = ledCount;

    uint32_t offset = pio_add_program(pio_, &ws2812_program);

    sm_ = 0;

    ws2812_program_init(
        pio_,
        sm_,
        offset,
        pin_,
        800000,
        false);
}

void LedController::setMode(
    ColorMode mode)
{
    mode_ = mode;
}

void LedController::setBrightness(
    float brightness)
{
    if(brightness < 0.0f)
        brightness = 0.0f;

    if(brightness > 1.0f)
        brightness = 1.0f;

    brightness_ = brightness;
}

void LedController::putPixel(
    uint32_t pixel)
{
    pio_sm_put_blocking(
        pio_,
        sm_,
        pixel << 8u);
}

uint32_t LedController::rgb(uint8_t r, uint8_t g, uint8_t b)
{
    r = (uint8_t)(r * brightness_);
    g = (uint8_t)(g * brightness_);
    b = (uint8_t)(b * brightness_);

    return urgb_u32(r, g, b);
}

void LedController::fill(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t color = rgb(r, g, b);

    for(uint32_t i = 0; i < ledCount_; i++)
    {
        putPixel(color);
    }
}

void LedController::showRainbow()
{
    for(uint32_t i = 0; i < ledCount_; i++)
    {
        uint8_t pos = (i * 256 / ledCount_) + rainbowOffset_;
        uint8_t r, g, b;

        if(pos < 85)
        {
            r = pos * 3;
            g = 255 - pos * 3;
            b = 0;
        }
        else if(pos < 170)
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
    for(uint32_t i = 0; i < ledCount_; i++)
    {
        uint8_t r = 255;
        uint8_t g = 20 + rand() % 150;

        putPixel(rgb(r, g, 0));
    }
}

void LedController::showPolice()
{
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if(now - animationTimer_ > 250)
    {
        animationTimer_ = now;
        policeState_ = !policeState_;
    }

    if(policeState_)
    {
        fill(255, 0, 0);
    }
    else
    {
        fill(0, 0, 255);
    }
}

void LedController::showBreathing()
{
    breathingPhase_ += 0.05f;

    if(breathingPhase_ > 6.28f)
        breathingPhase_ = 0;

    float k = (sinf(breathingPhase_) + 1.0f) * 0.5f;
    uint8_t value = (uint8_t)(255 * k);

    fill(value, value, value);
}

void LedController::update()
{
    switch(mode_)
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