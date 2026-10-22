#include "led-task.h"
#include "pico/stdlib.h"

static led_state_t led_state;
static uint64_t half_period_us;
static uint64_t last_toggle_us;

static bool period_elapsed(uint64_t *last_us, uint64_t period_us)
{
    uint64_t now_us = time_us_64();
    if (now_us - *last_us < period_us)
    {
        return false;
    }

    // Сохраняем сетку периода при небольшом опоздании.
    *last_us += period_us;
    if (now_us - *last_us >= period_us)
    {
        // Пропущенные переключения не догоняем.
        *last_us = now_us;
    }
    return true;
}

void led_task_init(void)
{
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    led_task_set_period_ms(1000);
    led_task_set_state(LED_STATE_BLINK);
}

void led_task_set_state(led_state_t state)
{
    led_state = state;
    switch (led_state)
    {
    case LED_STATE_OFF:
        gpio_put(PICO_DEFAULT_LED_PIN, false);
        break;
    case LED_STATE_ON:
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        break;
    case LED_STATE_BLINK:
        // Первое переключение произойдёт на ближайшем вызове обработчика.
        last_toggle_us = time_us_64() - half_period_us;
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

void led_task_handle(void)
{
    switch (led_state)
    {
    case LED_STATE_OFF:
    case LED_STATE_ON:
        break;
    case LED_STATE_BLINK:
        if (period_elapsed(&last_toggle_us, half_period_us))
        {
            gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
        }
        break;
    }
}
