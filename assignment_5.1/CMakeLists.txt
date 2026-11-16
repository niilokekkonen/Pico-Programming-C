# CMake for assignment 3

add_executable(asssignment_5.1
        main.c
        functions.c
        functions.h
)

target_link_libraries(asssignment_5.1
        pico_stdlib
        hardware_pwm
        hardware_gpio
        hardware_i2c
)

pico_enable_stdio_usb(asssignment_5.1 0)
pico_enable_stdio_uart(asssignment_5.1 1)

pico_add_extra_outputs(asssignment_5.1)