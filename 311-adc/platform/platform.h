#pragma once

#include <stdbool.h>

void platform_init(void);
void platform_led_set(bool on);
// Текущий уровень кнопки без подавления дребезга.
bool platform_button_read(void);

// Напряжение на входе АЦП GP26 в вольтах.
float platform_voltage_read(void);

// Температура кристалла по встроенному датчику в градусах Цельсия.
float platform_temp_read(void);
