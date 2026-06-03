#include "ServoController.h"
#include <stdint.h>
#include "hardware/pwm.h"
#include "hardware/gpio.h"

void ServoController::init(unsigned pin)
{
    pin_ = pin;

    gpio_set_function(pin_, GPIO_FUNC_PWM);

    uint32_t slice = pwm_gpio_to_slice_num(pin_);

    pwm_config cfg = pwm_get_default_config();

    pwm_config_set_clkdiv(&cfg, 125.0f);

    pwm_config_set_wrap(&cfg, 20000);

    pwm_init(slice, &cfg, true);

    writeAngle(currentAngle_);
}

void ServoController::setTarget(float angle)
{
    if(angle < 22.5f) angle = 0.0f;
    else if(angle > 45.0f) angle = 45.0f;
    else angle = (angle - 22.5f) * 2;

    targetAngle_ = angle;
}

float ServoController::getAngle() const
{
    return currentAngle_;
}

float ServoController::getCurrentAngle() const
{
    return currentAngle_;
}

void ServoController::update()
{
    currentAngle_ += (targetAngle_ - currentAngle_) * 0.02f;

    writeAngle(currentAngle_);
}

void ServoController::writeAngle(float angle)
{
    float min_pulse_us = 500.0f;
    float max_pulse_us = 2500.0f;

    float pulse_us = min_pulse_us + (angle / 180.0f) * (max_pulse_us - min_pulse_us);

    pwm_set_gpio_level(pin_, static_cast<uint32_t>(pulse_us));
}