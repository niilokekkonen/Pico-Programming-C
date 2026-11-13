# CMake for assignment 4

add_executable(assignment_4
        main.c
        iuart.c
        iuart.h
        functions.c
        functions.h
)

target_link_libraries(assignment_4
        pico_stdlib
        hardware_pwm
        hardware_gpio
)

pico_enable_stdio_usb(assignment_4 0)
pico_enable_stdio_uart(assignment_4 1)

pico_add_extra_outputs(assignment_4)