#include "led-task/led-task.h"
#include "systime/systime.h"
#include "platform/platform.h"

static led_state_t led_state;
static bool led_on;
static uint64_t half_period_us;
static uint64_t last_toggle_us;

static void led_set(bool on)
{
    led_on = on;
    platform_led_set(on);
}

void led_task_init(void)
{
    led_set(false);
    led_task_set_period_ms(1000);
    led_task_set_state(LED_STATE_BLINK);
}

void led_task_set_state(led_state_t state)
{
    led_state = state;
    switch (led_state)
    {
    case LED_STATE_OFF:
        led_set(false);
        break;
    case LED_STATE_ON:
        led_set(true);
        break;
    case LED_STATE_BLINK:
        // Первое переключение произойдёт на ближайшем вызове обработчика.
        last_toggle_us = systime_us() - half_period_us;
        break;
    }
}

void led_task_next_state(void)
{
    switch (led_state)
    {
    case LED_STATE_OFF:
        led_task_set_state(LED_STATE_ON);
        break;
    case LED_STATE_ON:
        led_task_set_state(LED_STATE_BLINK);
        break;
    case LED_STATE_BLINK:
        led_task_set_state(LED_STATE_OFF);
        break;
    }
}

led_state_t led_task_get_state(void)
{
    return led_state;
}

bool led_task_set_period_ms(uint32_t period_ms)
{
    if (period_ms == 0)
    {
        return false;
    }
    half_period_us = (uint64_t)period_ms * 1000 / 2;
    return true;
}

uint32_t led_task_get_period_ms(void)
{
    return (uint32_t)(half_period_us / 500);
}

void led_task_next_period(void)
{
    switch (led_task_get_period_ms())
    {
    case 250:
        led_task_set_period_ms(500);
        break;
    case 500:
        led_task_set_period_ms(1000);
        break;
    default:
        // После 1000 мс или произвольного периода из команды — начало круга.
        led_task_set_period_ms(250);
        break;
    }
}

void led_task_handle(void)
{
    switch (led_state)
    {
    case LED_STATE_OFF:
    case LED_STATE_ON:
        break;
    case LED_STATE_BLINK:
        if (systime_period_elapsed(&last_toggle_us, half_period_us))
        {
            led_set(!led_on);
        }
        break;
    }
}
