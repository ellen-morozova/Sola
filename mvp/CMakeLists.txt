cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(full_pico_smart_light)
pico_sdk_init()

add_executable(
    full_pico_smart_light
    src/main.cpp
    src/LightSensor.cpp
    src/ServoController.cpp
    src/UltrasonicSensor.cpp
    src/LedController.cpp
)

target_include_directories(
    full_pico_smart_light PRIVATE
    include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_pio/include
)

target_link_libraries(
    full_pico_smart_light
    pico_stdlib
    hardware_pwm
    hardware_adc
    hardware_pio
    hardware_clocks
)

pico_generate_pio_header(
    full_pico_smart_light
    ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio
)

pico_add_extra_outputs(full_pico_smart_light)
