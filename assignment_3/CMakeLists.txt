# CMake for assignment 3

add_executable(assignment_3
        main.c
        functions.c
        functions.h
)

target_link_libraries(assignment_3
        pico_stdlib
        hardware_pwm
        hardware_gpio
)

pico_enable_stdio_usb(assignment_3 0)
pico_enable_stdio_uart(assignment_3 1)

pico_add_extra_outputs(assignment_3)