#include "telemetry-task/telemetry-task.h"
#include "platform/platform.h"
#include "systime/systime.h"
#include <stddef.h>

static telemetry_state_t telemetry_state;
static telemetry_task_callback_t record_callback;
static uint32_t telemetry_period_ms;
static uint64_t last_record_us;

void telemetry_task_init(telemetry_task_callback_t on_record)
{
    record_callback = on_record;
    telemetry_state = TELEMETRY_STATE_IDLE;
    telemetry_period_ms = 100;
    last_record_us = 0;
}

bool telemetry_task_start(uint32_t period_ms)
{
    if (period_ms < 10)
    {
        return false;
    }
    telemetry_period_ms = period_ms;
    // Первая запись — на ближайшем вызове обработчика.
    last_record_us = systime_us() - (uint64_t)period_ms * 1000;
    telemetry_state = TELEMETRY_STATE_RUN;
    return true;
}

void telemetry_task_stop(void)
{
    telemetry_state = TELEMETRY_STATE_IDLE;
}

telemetry_state_t telemetry_task_get_state(void)
{
    return telemetry_state;
}

uint32_t telemetry_task_get_period_ms(void)
{
    return telemetry_period_ms;
}

void telemetry_task_handle(void)
{
    switch (telemetry_state)
    {
    case TELEMETRY_STATE_IDLE:
        break;
    case TELEMETRY_STATE_RUN:
        if (systime_period_elapsed(&last_record_us,
                                   (uint64_t)telemetry_period_ms * 1000))
        {
            telemetry_record_t record;
            record.time_us = systime_us();
            record.voltage_v = platform_voltage_read();
            record.temp_c = platform_temp_read();
            if (record_callback != NULL)
            {
                record_callback(&record);
            }
        }
        break;
    }
}
