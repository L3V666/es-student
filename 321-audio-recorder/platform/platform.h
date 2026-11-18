#pragma once

#include <stdbool.h>
#include <stdint.h>

// Мигание по будильнику; false — неверный период или нет свободного будильника.
bool platform_blink_start(uint32_t period_ms);
// Останавливает будильник, гасит светодиод и возвращает число переключений.
uint32_t platform_blink_stop(void);

void platform_init(void);
void platform_led_set(bool on);
// true — заряд RC-цепи с GP22, false — разряд через резистор.
void platform_charge_set(bool on);
// Текущий уровень кнопки без подавления дребезга.
bool platform_button_read(void);

// Напряжение на входе АЦП GP26 в вольтах.
float platform_voltage_read(void);

// Температура кристалла по встроенному датчику в градусах Цельсия.
float platform_temp_read(void);
