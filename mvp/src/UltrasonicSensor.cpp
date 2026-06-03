#include "UltrasonicSensor.h"
#include "pico/stdlib.h"

UltrasonicSensor::UltrasonicSensor(unsigned trig, unsigned echo)
    : trig_(trig), echo_(echo)
{
}

void UltrasonicSensor::init()
{
    gpio_init(trig_);
    gpio_set_dir(trig_, GPIO_OUT);
    gpio_put(trig_, 0);

    gpio_init(echo_);
    gpio_set_dir(echo_, GPIO_IN);

    ready_ = false;
    error_ = false;
    distance_ = 1.0f;
    gestureLatched_ = false;
}

void UltrasonicSensor::update()
{
    ready_ = false;
    error_ = false;

    gpio_put(trig_, 0);
    sleep_us(2);
    gpio_put(trig_, 1);
    sleep_us(10);
    gpio_put(trig_, 0);

    uint32_t start_wait = time_us_32();
    while (!gpio_get(echo_))
    {
        if (time_us_32() - start_wait > 30000)
        {
            error_ = true;
            distance_ = 1.0f;
            ready_ = true;
            return;
        }
    }

    uint32_t echo_start = time_us_32();

    while (gpio_get(echo_))
    {
        if (time_us_32() - echo_start > 30000)
        {
            error_ = true;
            distance_ = 1.0f;
            ready_ = true;
            return;
        }
    }

    uint32_t echo_end = time_us_32();
    uint32_t echo_time = echo_end - echo_start;

    distance_ = echo_time * 0.0343f / 2.0f;
    ready_ = true;
    error_ = false;
}

bool UltrasonicSensor::measurementReady() const
{
    return ready_;
}

float UltrasonicSensor::getDistance() const
{
    return distance_;
}

bool UltrasonicSensor::hasError() const
{
    return error_;
}

bool UltrasonicSensor::gestureDetected(float thresholdCm)
{
    if (!ready_)
        return false;

    if (error_)
        return false;

    uint32_t now = to_ms_since_boot(get_absolute_time());

    const float enterThreshold = thresholdCm;
    const float exitThreshold = thresholdCm + HYSTERESIS_CM;

    if (gestureLatched_)
    {
        if (distance_ >= exitThreshold)
        {
            gestureLatched_ = false;
        }

        return false;
    }

    if (distance_ > enterThreshold)
    {
        return false;
    }

    if (now - lastGestureTime_ < GESTURE_COOLDOWN_MS)
    {
        return false;
    }

    gestureLatched_ = true;
    lastGestureTime_ = now;

    return true;
}