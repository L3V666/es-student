#include "button-task/button-task.h"
#include "systime/systime.h"
#include "platform/platform.h"
#include <stddef.h>

static const uint64_t DEBOUNCE_US = 20 * 1000;
static const uint64_t LONG_PRESS_US = 1000 * 1000;

typedef enum {
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESS_DEBOUNCE,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASE_DEBOUNCE,
} button_state_t;

typedef struct
{
    button_state_t state;
    button_task_callback_t on_press;
    button_task_callback_t on_long_press;
    uint64_t transition_us;
    uint64_t pressed_us;
    uint32_t press_count;
    bool long_press_reported;
    bool initialized;
} button_t;

static button_t buttons[PLATFORM_BUTTON_COUNT];

void button_task_init(platform_button_t button, button_task_callback_t on_press,
                      button_task_callback_t on_long_press)
{
    if ((unsigned)button >= PLATFORM_BUTTON_COUNT)
    {
        return;
    }
    buttons[button] = (button_t){
        .state = BUTTON_STATE_RELEASED,
        .on_press = on_press,
        .on_long_press = on_long_press,
        .initialized = true,
    };
}

static void handle_button(platform_button_t id, uint64_t now_us)
{
    button_t *button = &buttons[id];
    bool pressed = platform_button_read(id);

    switch (button->state)
    {
    case BUTTON_STATE_RELEASED:
        if (pressed)
        {
            button->transition_us = now_us;
            button->state = BUTTON_STATE_PRESS_DEBOUNCE;
        }
        break;
    case BUTTON_STATE_PRESS_DEBOUNCE:
        if (!pressed)
        {
            button->state = BUTTON_STATE_RELEASED;
        }
        else if (now_us - button->transition_us >= DEBOUNCE_US)
        {
            button->state = BUTTON_STATE_PRESSED;
            button->press_count++;
            button->pressed_us = now_us;
            button->long_press_reported = false;
            if (button->on_long_press == NULL && button->on_press != NULL)
            {
                button->on_press();
            }
        }
        break;
    case BUTTON_STATE_PRESSED:
        if (!pressed)
        {
            button->transition_us = now_us;
            button->state = BUTTON_STATE_RELEASE_DEBOUNCE;
        }
        else if (button->on_long_press != NULL &&
                 !button->long_press_reported &&
                 now_us - button->pressed_us >= LONG_PRESS_US)
        {
            // Флаг сохраняется и при дребезге отпускания.
            button->long_press_reported = true;
            button->on_long_press();
        }
        break;
    case BUTTON_STATE_RELEASE_DEBOUNCE:
        if (pressed)
        {
            button->state = BUTTON_STATE_PRESSED;
        }
        else if (now_us - button->transition_us >= DEBOUNCE_US)
        {
            button->state = BUTTON_STATE_RELEASED;
            if (button->on_long_press != NULL &&
                !button->long_press_reported && button->on_press != NULL)
            {
                button->on_press();
            }
        }
        break;
    }
}

void button_task_handle(void)
{
    uint64_t now_us = systime_us();
    for (unsigned i = 0; i < PLATFORM_BUTTON_COUNT; i++)
    {
        if (buttons[i].initialized)
        {
            handle_button((platform_button_t)i, now_us);
        }
    }
}

bool button_task_is_pressed(platform_button_t button)
{
    if ((unsigned)button >= PLATFORM_BUTTON_COUNT)
    {
        return false;
    }
    return buttons[button].state == BUTTON_STATE_PRESSED ||
           buttons[button].state == BUTTON_STATE_RELEASE_DEBOUNCE;
}

uint32_t button_task_get_press_count(platform_button_t button)
{
    if ((unsigned)button >= PLATFORM_BUTTON_COUNT)
    {
        return 0;
    }
    return buttons[button].press_count;
}
