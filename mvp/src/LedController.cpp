#include "LedController.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#include <cstdlib>
#include <cmath>
#include <cstdint>

uint8_t LedController::lerp8(uint8_t a, uint8_t b, float t)
{
    return static_cast<uint8_t>(
        static_cast<float>(a) +
        (static_cast<float>(b) - static_cast<float>(a)) * t
    );
}

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
    if (mode_ != mode)
    {
        mode_ = mode;
        rainbowOffset_ = 0;
        animationTimer_ = 0;
        policeState_ = false;
        breathingPhase_ = 0.0f;
    }
}

void LedController::setBrightness(float brightness)
{
    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > 1.0f) brightness = 1.0f;

    brightness_ = brightness;
}

void LedController::setBrightnessFromServo(float angle)
{
    float servoFactor = angle / 45.0f;

    if (servoFactor < 0.0f) servoFactor = 0.0f;
    if (servoFactor > 1.0f) servoFactor = 1.0f;

    targetBrightness_ = servoFactor;
}

void LedController::putPixel(uint32_t pixel)
{
    pio_sm_put_blocking(pio_, sm_, pixel << 8u);
}

uint32_t LedController::rgb(uint8_t r, uint8_t g, uint8_t b)
{
    float finalBrightness = targetBrightness_;

    if (finalBrightness < 0.5f) finalBrightness = 0.0f;
    else if (finalBrightness > 1.0f) finalBrightness = 1.0f;
    else finalBrightness = (finalBrightness - 0.5f) * 2;

    finalBrightness = brightness_ * finalBrightness;

    r = (uint8_t)((float)r * finalBrightness);
    g = (uint8_t)((float)g * finalBrightness);
    b = (uint8_t)((float)b * finalBrightness);

    return urgb_u32(r, g, b);
}

void LedController::fill(uint8_t r, uint8_t g, uint8_t b)
{
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
        uint8_t r = 180 + rand() % 76;
        uint8_t g = 10 + rand() % 120;
        uint8_t b = rand() % 20;
        putPixel(rgb(r, g, b));
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
    if (ledCount_ == 1)
    {
        if (policeState_) fill(255, 0, 0);
        else fill(0, 0, 255);
        return;
    }

    for (uint32_t i = 0; i < ledCount_; i++)
    {
        bool firstHalf = (i < ledCount_ / 2);

        if (policeState_)
        {
            putPixel(firstHalf ? rgb(255, 0, 0) : rgb(0, 0, 255));
        }
        else
        {
            putPixel(firstHalf ? rgb(0, 0, 255) : rgb(255, 0, 0));
        }
    }
}

void LedController::showBreathing()
{
    breathingPhase_ += 0.05f;

    if (breathingPhase_ > 6.2831853f)
    {
        breathingPhase_ = 0.0f;
    }

    float k = (sinf(breathingPhase_) + 1.0f) * 0.5f;
    uint8_t value = (uint8_t)(255.0f * k);

    fill(value, value, value);
}

void LedController::showTwoColorTransition(const LedColor& color1, const LedColor& color2)
{
    const uint32_t HOLD_TIME = 5000;
    const uint32_t FADE_TIME = 2000;

    uint32_t cycle = HOLD_TIME + FADE_TIME + HOLD_TIME + FADE_TIME;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    uint32_t pos = now % cycle;
    uint8_t r, g, b;

    if (pos < HOLD_TIME)
    {
        r = color1.r;
        g = color1.g;
        b = color1.b;
    }
    else if (pos < HOLD_TIME + FADE_TIME)
    {
        float t = (float)(pos - HOLD_TIME) / (float)FADE_TIME;
        r = lerp8(color1.r, color2.r, t);
        g = lerp8(color1.g, color2.g, t);
        b = lerp8(color1.b, color2.b, t);
    }
    else if (pos < HOLD_TIME + FADE_TIME + HOLD_TIME)
    {
        r = color2.r;
        g = color2.g;
        b = color2.b;
    }
    else
    {
        float t = (float)(pos - HOLD_TIME - FADE_TIME - HOLD_TIME) / (float)FADE_TIME;
        r = lerp8(color2.r, color1.r, t);
        g = lerp8(color2.g, color1.g, t);
        b = lerp8(color2.b, color1.b, t);
    }

    fill(r, g, b);
}

void LedController::showThreeColorTransition(
    const LedColor& c1,
    const LedColor& c2,
    const LedColor& c3)
{
    const uint32_t HOLD_TIME = 3000;
    const uint32_t FADE_TIME = 1000;
    const LedColor colors[3] = {c1, c2, c3};

    uint32_t segment = HOLD_TIME + FADE_TIME;
    uint32_t cycle = segment * 3;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    uint32_t pos = now % cycle;
    int current = pos / segment;
    uint32_t local = pos % segment;

    if (local < HOLD_TIME)
    {
        fill(colors[current].r, colors[current].g, colors[current].b);
    }
    else
    {
        float t = (float)(local - HOLD_TIME) / (float)FADE_TIME;
        int next = (current + 1) % 3;
        uint8_t r = lerp8(colors[current].r, colors[next].r, t);
        uint8_t g = lerp8(colors[current].g, colors[next].g, t);
        uint8_t b = lerp8(colors[current].b, colors[next].b, t);
        fill(r, g, b);
    }
}

void LedController::showRandomPalette(const std::vector<LedColor>& palette)
{
    if (palette.empty())
        return;

    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (now - randomUpdateTime_ < 200) return;
    randomUpdateTime_ = now;
    for (uint32_t i = 0; i < ledCount_; i++)
    {
        const LedColor& c = palette[rand() % palette.size()];
        putPixel(rgb(c.r, c.g, c.b));
    }
}

void LedController::update()
{
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
        
        case ColorMode::PINKTOPURPLE:
            showTwoColorTransition({255, 56, 183}, {163, 33, 255});
            break;
        
        case ColorMode::ORANGETOBLUE:
            showTwoColorTransition({255, 139, 33}, {33, 186, 255});
            break;
        
        case ColorMode::BLUETOGREEN:
            showTwoColorTransition({5, 98, 255}, {33, 255, 211});
            break;
        
        case ColorMode::DARKPINKTOBLUETOORANGE:
            showThreeColorTransition({255, 18, 176}, {15, 196, 255}, {255, 171, 43});
            break;
        
        case ColorMode::PINKTOBLUETOORANGE:
            showThreeColorTransition({255, 156, 237}, {172, 242, 252}, {255, 211, 143});
            break;
        
        case ColorMode::SUN:
            showRandomPalette({{240, 171, 22}, {232, 79, 2}, {230, 61, 18}, {247, 29, 0}, {247, 0, 0}, {255, 201, 5}});
            break;
        
        case ColorMode::STROBERRY:
            showRandomPalette({{255, 155, 5}, {255, 41, 41}, {255, 0, 111}, {231, 45, 237}});
            break;
        
        case ColorMode::NIGHTSKY:
            showRandomPalette({{39, 6, 201}, {31, 120, 255}, {98, 0, 255}, {18, 188, 255}});
            break;
        
        case ColorMode::LEMON:
            showRandomPalette({{195, 255, 0}, {117, 255, 43}, {0, 219, 22}});
            break;
    }
}