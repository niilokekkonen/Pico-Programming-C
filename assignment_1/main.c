#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

bool pressed(uint btn_pin);

int main()
{
    // Led pins
    const uint led1_pin = 20;
    const uint led2_pin = 21;
    const uint led3_pin = 22;
    const uint32_t led_mask = (1u << led1_pin) | (1u << led2_pin) | (1u << led3_pin);
    
    // Btn pins
    const uint btn1_pin = 7; // SW2
    const uint btn2_pin = 8; // SW1
    const uint btn3_pin = 9; // SW0
    const uint32_t btn_mask = (1u << btn1_pin) | (1u << btn2_pin) | (1u << btn3_pin);

    // PWM variables
    const uint32_t clk_divider = 125;
    const uint16_t top = 999; // Wrap with max value 2^16 - 1
    uint16_t lvl = 500;

    // To keep track of pressing
    uint times_toggled = 0;
    uint led_state = 0;
    bool btn_1_cur_state = false;
    bool btn_1_prev_state = false;
    bool btn_2_cur_state = false; // Current state read from GPIO
    bool btn_2_prev_state = false; // Prev state to keep track of btn presses
    bool btn_3_cur_state = false;
    bool btn_3_prev_state = false;

    // Initialize LED pins with mask
    gpio_init_mask(led_mask);
    gpio_set_dir_masked(led_mask, led_mask);

    // Init PWM configuration
    pwm_config conf = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&conf, clk_divider);
    pwm_config_set_wrap(&conf, top);

    // Initialize PWM on all LEDs, and fetch slices + channels
    gpio_set_function_masked(led_mask, GPIO_FUNC_PWM);
    uint led1_slice = pwm_gpio_to_slice_num(led1_pin);
    uint led2_slice = pwm_gpio_to_slice_num(led2_pin);
    uint led3_slice = pwm_gpio_to_slice_num(led3_pin);
    uint led1_channel = pwm_gpio_to_channel(led1_pin);
    uint led2_channel = pwm_gpio_to_channel(led2_pin);
    uint led3_channel = pwm_gpio_to_channel(led3_pin);

    // Initialize PWM on slices, and set channel lvls
    pwm_init(led1_slice, &conf, false);
    pwm_init(led2_slice, &conf, false);
    pwm_init(led3_slice, &conf, false);
    pwm_set_chan_level(led1_slice, led1_channel, 0);
    pwm_set_chan_level(led2_slice, led2_channel, 0);
    pwm_set_chan_level(led3_slice, led3_channel, 0);


    // Init button pins
    gpio_init_mask(btn_mask);
    gpio_set_dir_masked(btn_mask, 0);
    // define buttons as pull up, Pressed = 0, released = 1 (hence pull up)
    gpio_pull_up(btn1_pin);
    gpio_pull_up(btn2_pin);
    gpio_pull_up(btn3_pin);

    // Initialize chosen serial port
    stdio_init_all();
    // Initialize PWM slices to zero
    pwm_set_enabled(led1_slice, true);
    pwm_set_enabled(led2_slice, true);
    pwm_set_enabled(led3_slice, true);
    while (1)
    {
        const uint16_t step = 100;
        // Checking button 1
        btn_1_cur_state = pressed(btn1_pin);
        btn_2_cur_state = pressed(btn2_pin);
        btn_3_cur_state = pressed(btn3_pin);

        // If SW2 is pressed, and led is on decrease brightness
        if (btn_1_cur_state && !btn_1_prev_state && led_state)
        {
            if (lvl <= step)
            {
                lvl = 0;
            }
            else
            {
                lvl -= step;
            }
            pwm_set_chan_level(led1_slice, led1_channel,lvl);
            pwm_set_chan_level(led2_slice, led2_channel,lvl);
            pwm_set_chan_level(led3_slice, led3_channel,lvl);
            printf("Pressed SW2 \n");

        }

        if (btn_2_cur_state && !btn_2_prev_state) {
            if (led_state && lvl == 0)
            {
                lvl = 500; // 50% of the PWM level
                pwm_set_chan_level(led1_slice, led1_channel,lvl);
                pwm_set_chan_level(led2_slice, led2_channel,lvl);
                pwm_set_chan_level(led3_slice, led3_channel,lvl);
            }
            else
            {
                // Led 1 state gets reversed everytime the button is pressed
                led_state = !led_state;
                if (led_state == 1)
                {
                    pwm_set_chan_level(led1_slice, led1_channel,lvl);
                    pwm_set_chan_level(led2_slice, led2_channel,lvl);
                    pwm_set_chan_level(led3_slice, led3_channel,lvl);
                }
                else
                {
                    pwm_set_chan_level(led1_slice, led1_channel,0);
                    pwm_set_chan_level(led2_slice, led2_channel,0);
                    pwm_set_chan_level(led3_slice, led3_channel,0);
                }

            }
        }

        // If SW0 is pressed, and led is toggled increase brightness
        if (btn_3_cur_state && !btn_3_prev_state && led_state) {
            if (lvl >= top + 1)
            {
                lvl = top + 1; // Setting the value to max if it exceeds the max value
            }
            else
            {
                lvl += step;
            }
            pwm_set_chan_level(led1_slice, led1_channel,lvl);
            pwm_set_chan_level(led2_slice, led2_channel,lvl);
            pwm_set_chan_level(led3_slice, led3_channel,lvl);
            printf("SW0 PRESSED: \n");
        }
        btn_1_prev_state = btn_1_cur_state;
        btn_2_prev_state = btn_2_cur_state; // Saving the results
        btn_3_prev_state = btn_3_cur_state;
        printf("PWM LEVEL: %u\n", lvl);
        sleep_ms(10);
    }
}

// returns true if button is pressed, else false
// Uses pull up resistor button
bool pressed(uint btn_pin)
{
    uint pressed = 0;
    uint release = 0;
    // Loop forever
    while (pressed < 3 && release < 3)
    {
        // Blink LED if button is pressed
        if (!gpio_get(btn_pin))
        {
            pressed++;
            release = 0;
        }
        else
        {
            release++;
            pressed = 0;
        }
    }
    if(pressed > release) return true;
    else return false;
}