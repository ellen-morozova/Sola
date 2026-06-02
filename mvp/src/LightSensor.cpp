#include "LightSensor.h"
#include "hardware/adc.h"

void LightSensor::init() {
    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);
}

uint16_t LightSensor::read(){
    buf[pos] = adc_read(); 
    pos = (pos + 1) % N;
    uint32_t s = 0; 
    for(auto v : buf) {
        s += v;
    }
    return s / N;
}
