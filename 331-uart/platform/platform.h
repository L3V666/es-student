#pragma once

#include <stdbool.h>

void platform_init(void);
void platform_led_set(bool on);
// Текущий уровень кнопки без подавления дребезга.
bool platform_button_read(void);

// Режим драйвера линии RC-цепи на GP22.
typedef enum {
    PLATFORM_LINE_PUSH_PULL,
    PLATFORM_LINE_OPEN_DRAIN,
} platform_line_mode_t;

void platform_line_set(platform_line_mode_t mode, bool high);
// Напряжение узла RC-цепи на GP26, в вольтах.
float platform_voltage_read(void);
