#include "ServoController.h"

#include "hardware/pwm.h"
#include "hardware/gpio.h"

void ServoController::init(unsigned pin)
{
    pin_ = pin;

    gpio_set_function(pin_, GPIO_FUNC_PWM);

    uint32_t slice = pwm_gpio_to_slice_num(pin_);

    pwm_config cfg = pwm_get_default_config();

    pwm_config_set_clkdiv(&cfg, 64.0f);

    pwm_init(slice, &cfg, true);

    writeAngle(currentAngle_);
}

void ServoController::setTarget(float angle)
{
    if(angle < 0.0f)
        angle = 0.0f;

    if(angle > 180.0f)
        angle = 180.0f;

    targetAngle_ = angle;
}

float ServoController::getAngle() const
{
    return currentAngle_;
}

void ServoController::update()
{
    currentAngle_ += (targetAngle_ - currentAngle_) * 0.05f;

    writeAngle(currentAngle_);
}

void ServoController::writeAngle(float angle)
{
    float pulse_us = 500.0f + angle * (2000.0f / 180.0f);

    uint32_t level = static_cast<uint32_t>(pulse_us * 65535.0f / 20000.0f);

    pwm_set_gpio_level(pin_, level);
}