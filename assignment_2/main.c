#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico/util/queue.h"

// Rotary pins
#define ROTSW_PIN 12
#define ROT_A_PIN 10
#define ROT_B_PIN 11
#define DEBOUNCE_WINDOW 150000 // unit is microseconds, = 150 ms

// LED pins
#define LED1_PIN 20
#define LED2_PIN 21
#define LED3_PIN 22
#define QUEUE_SIZE 32

typedef enum
{
    LED_ON,
    LED_OFF,
} led_states; // Adjusting = twisting rotary knob

typedef enum
{
    EVENT_CW,
    EVENT_CCW,
    EVENT_SW
} io_events;
void set_leds(uint led1_pin, uint led2_pin, uint led3_pin, uint level);
uint16_t increase_level(uint16_t level, uint16_t step, uint16_t wrap);
uint16_t decrease_level(uint16_t level, uint16_t step);
void isr_handler(uint gpio, uint32_t events);
bool debounce_btn(uint32_t cur_time, uint32_t prev_accept_time);

static queue_t rot_events; // Queue to store rotary events

int main()
{
    // 32 bit mask to toggle LEDs, and buttons
    const uint32_t led_mask = (1u << LED1_PIN) | (1u << LED2_PIN) | (1u << LED3_PIN);
    const uint32_t btn_mask = (1u << ROT_A_PIN) | (1u << ROT_B_PIN) | (1u << ROTSW_PIN);

    // PWM variables
    const uint32_t clk_divider = 125;
    const uint16_t top = 999; // Wrap with max value 2^16 - 1
    uint16_t lvl = 500;

    // To keep track of states.
    queue_init(&rot_events, sizeof(io_events), QUEUE_SIZE);
    // To debounce rot_SW
    uint32_t last_sw_press = 0;
    led_states current_state = LED_OFF; // Starting from off state
    io_events rot_event;

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
    // Set masks, as inputs
    gpio_set_dir_masked(btn_mask, 0);
    // define rot SW as pull up, Pressed = 0, released = 1 (hence pull up)
    gpio_pull_up(ROTSW_PIN);

    // Initialize chosen serial port
    stdio_init_all();
    // Start PWM
    pwm_set_enabled(led1_slice, true);
    pwm_set_enabled(led3_slice, true);
    // Setting interrupt for rot_SW, and rot_A
    gpio_set_irq_enabled_with_callback(ROTSW_PIN, GPIO_IRQ_EDGE_FALL, true, &isr_handler);
    gpio_set_irq_enabled(ROT_A_PIN, GPIO_IRQ_EDGE_RISE, true);
    while (1)
    {
        while (queue_try_remove(&rot_events, &rot_event))
        {
            bool accepted = true; // Will the event be handled
            if (rot_event == EVENT_SW)
            {
                uint32_t cur_press = time_us_32();
                accepted = debounce_btn(cur_press, last_sw_press);
                // Checking debounce, and SW pin incase of bouncing
                if (accepted && !gpio_get(ROTSW_PIN))
                {
                    last_sw_press = cur_press;
                    // Accepted = true
                }
                else
                {
                    accepted = false;
                }
            }
            const uint16_t step = 15;
            // Check buttons
            // Switch case for states
            // STATES = OFF, ON
            if (accepted)
            {
                switch (current_state)
                {
                    case LED_OFF:
                        if (rot_event == EVENT_SW)
                        {
                            current_state = LED_ON;
                            set_leds(LED1_PIN, LED2_PIN, LED3_PIN, lvl);
                        }
                        break;
                    case LED_ON:
                        if (rot_event == EVENT_SW)
                        {
                            if (lvl == 0)
                            {
                                current_state = LED_ON;
                                lvl = (top + 1) / 2;
                                set_leds(LED1_PIN, LED2_PIN, LED3_PIN, lvl);
                            }
                            else
                            {
                                current_state = LED_OFF;
                                set_leds(LED1_PIN, LED2_PIN, LED3_PIN, 0);
                            }
                        }
                        // Clockwise rotation
                        if (rot_event == EVENT_CW)
                        {
                            lvl = increase_level(lvl, step, top);
                            set_leds(LED1_PIN, LED2_PIN, LED3_PIN, lvl);
                        }
                        // Counter Clockwise rotation
                        if (rot_event == EVENT_CCW)
                        {
                            lvl = decrease_level(lvl, step);
                            set_leds(LED1_PIN, LED2_PIN, LED3_PIN, lvl);
                        }
                        break;


            }

            }
            // TEST -> printf("PWM LEVEL: %u\n", lvl);
        }
        sleep_ms(10);
    }
}

// Set level on 3 LEDs
void set_leds(uint led1_pin, uint led2_pin, uint led3_pin, uint level)
{
    pwm_set_gpio_level(led1_pin, level);
    pwm_set_gpio_level(led2_pin, level);
    pwm_set_gpio_level(led3_pin, level);
}

// Toggles LEDs when SW 1 is pressed
void rot_sw_toggle(uint led_state, uint level, uint led1_pin, uint led2_pin, uint led3_pin)
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

// Checks rotary encoders events, and sends them to the queue
void isr_handler(uint gpio, uint32_t events)
{
    io_events event;
    // printf("Entered isr handler\n");
    if (gpio == ROTSW_PIN)
    {
        event = EVENT_SW;
        queue_try_add(&rot_events, &event);
    }
    if (gpio == ROT_A_PIN)
    {
        if (gpio_get(ROT_B_PIN))
        {
            event = EVENT_CCW;
        }
        else
        {
            event = EVENT_CW;
        }
        queue_try_add(&rot_events, &event);
    }
}

// Returns true if debounce is good, else false
bool debounce_btn(uint32_t cur_time, uint32_t prev_accept_time)
{
    if (cur_time - prev_accept_time >= DEBOUNCE_WINDOW)
    {
        return true;
    }
    return false;
}