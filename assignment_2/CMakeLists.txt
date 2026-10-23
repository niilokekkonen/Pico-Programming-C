# CMake for assignment 2

add_executable(assignment_2
        main.c
)

target_link_libraries(assignment_2
        pico_stdlib
        hardware_pwm
        hardware_gpio
)

pico_enable_stdio_usb(assignment_2 0)
pico_enable_stdio_uart(assignment_2 1)

pico_add_extra_outputs(assignment_2)