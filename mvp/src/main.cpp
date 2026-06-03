#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/pio.h"

#include "config.h"
#include "LightSensor.h"
#include "ServoController.h"
#include "UltrasonicSensor.h"
#include "LedController.h"

int main()
{
    stdio_init_all();
    // sleep_ms(1000);

    // gpio_init(SWITCH_PIN);
    // gpio_set_dir(SWITCH_PIN, GPIO_IN);
    // gpio_pull_up(SWITCH_PIN);

    LightSensor lightSensor;
    lightSensor.init();

    ServoController servo;
    servo.init(SERVO_PIN);

    UltrasonicSensor modeSensor(TRIG1, ECHO1);
    UltrasonicSensor brightSensor(TRIG2, ECHO2);

    modeSensor.init();
    brightSensor.init();

    LedController leds;
    leds.init(pio0, WS2812_PIN, WS2812_COUNT);

    int brightnessMode = 3;
    const float brightnesses[] =
    {
        0.25f,
        0.50f,
        0.75f,
        1.00f
    };

    int colorMode = 0;
    leds.setMode(static_cast<ColorMode>(colorMode));
    leds.setBrightness(brightnesses[brightnessMode]);

    while (true)
    {
        // if (gpio_get(SWITCH_PIN))
        // {
        //     servo.update();
        //     leds.update();
        //     sleep_ms(20);
        //     continue;
        // }

        uint16_t light = lightSensor.read();

        float targetAngle = light * 45.0f / 4095.0f;
        servo.setTarget(targetAngle);
        servo.update();

        float currentAngle = servo.getCurrentAngle();
        leds.setBrightnessFromServo(currentAngle);

        modeSensor.update();
        brightSensor.update();

        if (modeSensor.gestureDetected(DETECT_DISTANCE_CM))
        {
            colorMode = (colorMode + 1) % COLOR_MODE_COUNT;
            leds.setMode(static_cast<ColorMode>(colorMode));
        }

        if (brightSensor.gestureDetected(DETECT_DISTANCE_CM))
        {
            brightnessMode = (brightnessMode + 1) % 4;
            leds.setBrightness(brightnesses[brightnessMode]);
        }

        leds.update();
        sleep_ms(20);
    }

    return 0;
}