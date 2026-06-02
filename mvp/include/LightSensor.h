#pragma once
#include <cstdint>

class LightSensor 
{
public:
    void init();
    uint16_t read();
private:
    static constexpr int N = 16;
    uint16_t buf[N]{};
    int pos = 0;
};
