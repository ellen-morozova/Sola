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
    unsigned trig_;
    unsigned echo_;

    bool ready_ = false;
    bool error_ = false;

    float distance_ = 0.0f;

    uint32_t lastGestureTime_ = 0;

    bool gestureLatched_ = false;

    static constexpr uint32_t GESTURE_COOLDOWN_MS = 500;
    static constexpr float HYSTERESIS_CM = 2.0f;
};