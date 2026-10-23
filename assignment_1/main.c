#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

// Button pins
#define SW0_PIN 9
#define SW1_PIN 8
#define SW2_PIN 7
// LED pins
#define LED1_PIN 20
#define LED2_PIN 21
#define LED3_PIN 22

static bool pressed(uint btn_pin);
static void SW1_toggle(uint led_state, uint level, uint led1_pin, uint led2_pin, uint led3_pin);
static void set_leds(uint led1_pin, uint led2_pin, uint led3_pin, uint level);
static uint16_t increase_level(uint16_t level, uint16_t step, uint16_t wrap);
static uint16_t decrease_level(uint16_t level, uint16_t step);

int main()
{
    // 32 bit mask to toggle LEDs, and buttons
    const uint32_t led_mask = (1u << LED1_PIN) | (1u << LED2_PIN) | (1u << LED3_PIN);
    const uint32_t btn_mask = (1u << SW0_PIN) | (1u << SW1_PIN) | (1u << SW2_PIN);

    // PWM variables
    const uint32_t clk_divider = 125;
    const uint16_t top = 999; // Wrap with max value 2^16 - 1
    uint16_t lvl = 500;

    // To keep track of pressing
    uint8_t led_state = 0; // 0 = off, 1 = on
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
    uint led1_slice = pwm_gpio_to_slice_num(LED1_PIN); // Slice = 2
    uint led2_slice = pwm_gpio_to_slice_num(LED2_PIN); // Slice = 2
    uint led3_slice = pwm_gpio_to_slice_num(LED3_PIN); // Slice = 3

    // Initialize PWM on slices, and set channel lvls
    pwm_init(led1_slice, &conf, false);
    pwm_init(led2_slice, &conf, false);
    pwm_init(led3_slice, &conf, false); // Note: if changing pins, check the slices
    set_leds(LED1_PIN, LED2_PIN, LED3_PIN, 0);

    // Init button pins
    gpio_init_mask(btn_mask);
    gpio_set_dir_masked(btn_mask, 0);
    // define buttons as pull up, Pressed = 0, released = 1 (hence pull up)
    gpio_pull_up(SW0_PIN);
    gpio_pull_up(SW1_PIN);
    gpio_pull_up(SW2_PIN);

    // Initialize chosen serial port
    stdio_init_all();
    // Start PWM
    pwm_set_enabled(led1_slice, true);
    pwm_set_enabled(led3_slice, true);
    while (1)
    {
        const uint16_t step = 15;
        // Check buttons
        
        sw_0_cur_state = pressed(SW0_PIN);
        sw_1_cur_state = pressed(SW1_PIN);
        sw_2_cur_state = pressed(SW2_PIN);

        // If SW2 is pressed and led is on, decrease brightness
        if (sw_2_cur_state && led_state)
        {
            lvl = decrease_level(lvl, step);
            set_leds(LED1_PIN, LED2_PIN, LED3_PIN, lvl);
            // TEST -> printf("Pressed SW2 while LED is on\n");
        }
        // IF SW1 is pressed, and wasn't pressed
        if (sw_1_cur_state && !sw_1_prev_state)
        {
            if (led_state && lvl == 0)
            {
                lvl = (top + 1) / 2; // 50% of the PWM level
                set_leds(LED1_PIN, LED2_PIN, LED3_PIN, lvl);
            }
            else
            {
                // Led 1 state gets reversed everytime the button is pressed
                led_state = !led_state;
                SW1_toggle(led_state, lvl, LED1_PIN, LED2_PIN, LED3_PIN);
                // TEST -> printf("Pressed SW1 \n");
            }
        }

        // If SW0 is pressed and led is on, increase brightness
        if (sw_0_cur_state && led_state)
        {
            lvl = increase_level(lvl, step, top);
            set_leds(LED1_PIN, LED2_PIN, LED3_PIN, lvl);
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

// Increases 16 bit level, and returns it
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