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

// Захват GP26 по будильнику; АЦП недоступен до завершения.
#define PLATFORM_CAPTURE_MAX 2000
bool platform_adc_busy(void);
bool platform_capture_start(uint32_t count, uint32_t period_us);
bool platform_capture_done(void);

typedef struct {
    const uint16_t *samples;
    uint32_t count;
    uint32_t period_us;
    uint32_t min_interval_us;
    uint32_t max_interval_us;
    float handler_percent;
} platform_capture_t;

// Результат доступен после завершения и до следующего захвата.
void platform_capture_get(platform_capture_t *capture);
