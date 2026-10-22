# CMake for assignment 1

add_executable(assignment_1
        main.c
        funcs.h
)

target_link_libraries(assignment_1
        pico_stdlib
        hardware_pwm
        hardware_gpio
)

pico_enable_stdio_usb(assignment_1 0)
pico_enable_stdio_uart(assignment_1 1)

pico_add_extra_outputs(assignment_1)