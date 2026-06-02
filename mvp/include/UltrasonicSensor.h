#pragma once
#include <stdint.h>
#include "pico/stdlib.h"

class UltrasonicSensor
{
public:
    UltrasonicSensor(unsigned trig, unsigned echo);

    void init();

    void update();

    bool measurementReady() const;

    float getDistance() const;

    bool hasError() const;

    bool gestureDetected(float thresholdCm);

private:
    enum class State
    {
        IDLE,
        TRIGGER_HIGH,
        WAIT_RISE,
        WAIT_FALL
    };

    unsigned trig_;
    unsigned echo_;

    State state_ = State::IDLE;

    uint32_t stateStart_ = 0;
    uint32_t echoStart_ = 0;

    float distance_ = 0.0f;

    bool ready_ = false;
    bool error_ = false;

    uint32_t lastGestureTime_ = 0;

    static constexpr uint32_t GESTURE_COOLDOWN_MS = 500;
};