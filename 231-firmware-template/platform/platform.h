#pragma once

#include <stdbool.h>

void platform_init(void);
void platform_led_set(bool on);
// Текущий уровень кнопки без подавления дребезга.
bool platform_button_read(void);
