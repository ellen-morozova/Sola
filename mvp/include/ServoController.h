#pragma once

class ServoController
{
public:
    void init(unsigned pin);

    void setTarget(float angle);

    void update();

    float getAngle() const;

private:
    unsigned pin_;

    float currentAngle_ = 90.0f;
    float targetAngle_ = 90.0f;

    void writeAngle(float angle);
};