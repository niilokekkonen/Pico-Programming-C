#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

bool pressed(uint btn_pin);
void SW1_toggle(uint led_state, uint level, uint led1_pin, uint led2_pin, uint led3_pin);
void set_leds(uint led1_pin, uint led2_pin, uint led3_pin, uint level);
uint16_t increase_level(uint16_t level, uint16_t step, uint16_t wrap);
uint16_t decrease_level(uint16_t level, uint16_t step);

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
    uint led_state = 0;
    bool sw_2_cur_state = false;
    bool sw_1_cur_state = false; // Current state read from GPIO
    bool sw_1_prev_state = false; // Prev state to keep track of btn presses
    bool sw_0_cur_state = false;

    // Initialize LED pins with mask
    gpio_init_mask(led_mask);
    gpio_set_dir_masked(led_mask, led_mask);

    // Init PWM configuration
    pwm_config conf = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&conf, clk_divider);
    pwm_config_set_wrap(&conf, top);

    // Initialize PWM on all LEDs, and fetch slices + channels
    // LEDs 1 and 2 are on the same slice, but have different channels
    gpio_set_function_masked(led_mask, GPIO_FUNC_PWM);
    uint led1_slice = pwm_gpio_to_slice_num(led1_pin); // Slice = 2
    uint led2_slice = pwm_gpio_to_slice_num(led2_pin); // Slice = 2
    uint led3_slice = pwm_gpio_to_slice_num(led3_pin); // Slice = 3
    uint led1_channel = pwm_gpio_to_channel(led1_pin); // Chan = A
    uint led2_channel = pwm_gpio_to_channel(led2_pin); // Chan = B
    uint led3_channel = pwm_gpio_to_channel(led3_pin); // Chan = A


    // Initialize PWM on slices, and set channel lvls
    pwm_init(led1_slice, &conf, false); // Initializing slice 2 only once
    pwm_init(led3_slice, &conf, false); // Note: if changing pins, check the slices accordingly
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
    // Init slices
    pwm_set_enabled(led1_slice, true);
    pwm_set_enabled(led3_slice, true);
    while (1)
    {
        const uint16_t step = 15;
        // Check buttons
        sw_2_cur_state = pressed(btn1_pin);
        sw_1_cur_state = pressed(btn2_pin);
        sw_0_cur_state = pressed(btn3_pin);

        // If SW2 is pressed and led is on, decrease brightness
        if (sw_2_cur_state && led_state)
        {
            lvl = decrease_level(lvl, step);
            set_leds(led1_pin, led2_pin, led3_pin, lvl);
            // TEST -> printf("Pressed SW2 while LED is on\n");

        }

        // IF SW1 is pressed, and wasn't pressed
        if (sw_1_cur_state && !sw_1_prev_state)
        {
            if (led_state && lvl == 0)
            {
                lvl = (top + 1) / 2; // 50% of the PWM level
                set_leds(led1_pin, led2_pin, led3_pin, lvl);
            }
            else
            {
                // Led 1 state gets reversed everytime the button is pressed
                led_state = !led_state;
                SW1_toggle(led_state, lvl, led1_pin, led2_pin, led3_pin);
                // TEST -> printf("Pressed SW1 \n");
            }
        }

        // If SW0 is pressed and led is on, increase brightness
        if (sw_0_cur_state && led_state)
        {
            lvl = increase_level(lvl, step, top);
            set_leds(led1_pin, led2_pin, led3_pin, lvl);
            // TEST -> printf("PRESSED SW0 while LED is on\n");
        }
        sw_1_prev_state = sw_1_cur_state; // Saving the prev state
        // TEST -> printf("PWM LEVEL: %u\n", lvl); 
        sleep_ms(10);
    }
}

// returns true if button is pressed, else false
// Uses pull up resistor button
bool pressed(uint btn_pin)
{
    uint press_cnt = 0;
    uint release_cnt = 0;
    while (press_cnt < 3 && release_cnt < 3)
    {
        if (!gpio_get(btn_pin))
        {
            press_cnt++;
            release_cnt = 0;
        }
        else
        {
            release_cnt++;
            press_cnt = 0;
        }
    }
    if(press_cnt > release_cnt) return true;
    return false;
}

// Set level on 3 LEDs
void set_leds(uint led1_pin, uint led2_pin, uint led3_pin, uint level)
{
    pwm_set_gpio_level(led1_pin, level);
    pwm_set_gpio_level(led2_pin, level);
    pwm_set_gpio_level(led3_pin, level);
}

// Toggles LEDs when SW 1 is pressed
void SW1_toggle(uint led_state, uint level, uint led1_pin, uint led2_pin, uint led3_pin)
{
    if (led_state == 1)
    {
        set_leds(led1_pin, led2_pin, led3_pin, level);
    }
    else
    {
        set_leds(led1_pin, led2_pin, led3_pin, 0);
    }
}

// increases 16 bit level, and returns it
uint16_t increase_level(uint16_t level, uint16_t step, uint16_t wrap)
{

    if (level + step >= wrap + 1)
    {
        level = wrap + 1; // Upper limit
        return level;
    }
    level += step;
    return level;
}

// Decreases 16 bit level, and returns it
uint16_t decrease_level(uint16_t level, uint16_t step)
{
    if (level <= step)
    {
        level = 0; // Lower limit
        return level;
    }
    level -= step;
    return level;
}