#include "UltrasonicSensor.h"

#include "pico/stdlib.h"

UltrasonicSensor::UltrasonicSensor(unsigned trig, unsigned echo) : trig_(trig), echo_(echo)
{
}

void UltrasonicSensor::init()
{
    gpio_init(trig_);
    gpio_set_dir(trig_, GPIO_OUT);

    gpio_init(echo_);
    gpio_set_dir(echo_, GPIO_IN);

    gpio_put(trig_, 0);

    state_ = State::IDLE;
}

void UltrasonicSensor::update()
{
    uint32_t now = time_us_32();

    switch(state_)
    {
        case State::IDLE:
        {
            ready_ = false;

            gpio_put(trig_, 1);

            stateStart_ = now;

            state_ = State::TRIGGER_HIGH;

            break;
        }

        case State::TRIGGER_HIGH:
        {
            if(now - stateStart_ >= 10)
            {
                gpio_put(trig_, 0);

                stateStart_ = now;

                state_ = State::WAIT_RISE;
            }

            break;
        }

        case State::WAIT_RISE:
        {
            if(gpio_get(echo_))
            {
                echoStart_ = now;

                state_ = State::WAIT_FALL;
            }

            else if(now - stateStart_ > 30000)
            {
                error_ = true;

                distance_ = -1;

                ready_ = true;

                state_ = State::IDLE;
            }

            break;
        }

        case State::WAIT_FALL:
        {
            if(!gpio_get(echo_))
            {
                uint32_t echoTime = now - echoStart_;

                distance_ = echoTime * 0.0343f / 2.0f;

                ready_ = true;

                error_ = false;

                state_ = State::IDLE;
            }

            else if(now - echoStart_ > 30000)
            {
                error_ = true;

                distance_ = -1;

                ready_ = true;

                state_ = State::IDLE;
            }

            break;
        }
    }
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

bool UltrasonicSensor::gestureDetected(
    float thresholdCm)
{
    if(!ready_)
    {
        return false;
    }

    if(error_)
    {
        return false;
    }

    uint32_t now = to_ms_since_boot(get_absolute_time());

    if(distance_ > thresholdCm)
    {
        return false;
    }

    if(now - lastGestureTime_ < GESTURE_COOLDOWN_MS)
    {
        return false;
    }

    lastGestureTime_ = now;

    return true;
}