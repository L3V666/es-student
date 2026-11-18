#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    TELEMETRY_STATE_IDLE,
    TELEMETRY_STATE_RUN,
} telemetry_state_t;

typedef struct {
    uint64_t time_us;
    float voltage_v;
    float temp_c;
} telemetry_record_t;

// Запись действительна только во время вызова функции.
typedef void (*telemetry_task_callback_t)(const telemetry_record_t *record);

void telemetry_task_init(telemetry_task_callback_t on_record);
void telemetry_task_handle(void);
// false — период меньше 10 мс; текущий режим остаётся прежним.
bool telemetry_task_start(uint32_t period_ms);
void telemetry_task_stop(void);
telemetry_state_t telemetry_task_get_state(void);
uint32_t telemetry_task_get_period_ms(void);
