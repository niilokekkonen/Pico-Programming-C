# CMake for assignment 3

add_executable(assignment_5.1
        main.c
        eeprom_ledstate.c
        eeprom_ledstate.h
)

target_link_libraries(assignment_5.1
        pico_stdlib
        hardware_pwm
        hardware_gpio
        hardware_i2c
)

pico_enable_stdio_usb(assignment_5.1 0)
pico_enable_stdio_uart(assignment_5.1 1)

pico_add_extra_outputs(assignment_5.1)