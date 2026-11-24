# CMake for assignment 3

add_executable(assignment_5.2
        main.c
        eeprom_log.c
        eeprom_log.h
)

target_link_libraries(assignment_5.2
        pico_stdlib
        hardware_pwm
        hardware_gpio
        hardware_i2c
)

pico_enable_stdio_usb(assignment_5.2 0)
pico_enable_stdio_uart(assignment_5.2 1)

pico_add_extra_outputs(assignment_5.2)