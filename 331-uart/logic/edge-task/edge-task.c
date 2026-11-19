#include "edge-task/edge-task.h"
#include "systime/systime.h"
#include <stddef.h>

#define EMPTY_V 0.05f
#define RISE_V 2.0f
#define FULL_V 3.2f
#define FALL_V 1.3f
#define TIMEOUT_US 2000000

typedef enum {
    EDGE_STATE_IDLE,
    EDGE_STATE_DISCHARGE,
    EDGE_STATE_RISE,
    EDGE_STATE_CHARGE,
    EDGE_STATE_FALL,
} edge_state_t;

static edge_state_t state;
static platform_line_mode_t line_mode;
static edge_task_callback_t done_callback;
static uint64_t phase_start_us;
static uint32_t rise_us;

static void enter(edge_state_t next, bool high)
{
    platform_line_set(line_mode, high);
    phase_start_us = systime_us();
    state = next;
}

static void finish(bool ok, uint32_t fall_us)
{
    edge_task_callback_t callback = done_callback;
    platform_line_mode_t mode = line_mode;
    uint32_t rise = rise_us;
    // После завершения, включая timeout, оставляем линию разряжаться.
    platform_line_set(mode, false);
    done_callback = NULL;
    state = EDGE_STATE_IDLE;
    callback(mode, ok, rise, fall_us);
}

bool edge_task_start(platform_line_mode_t mode, edge_task_callback_t on_done)
{
    if (state != EDGE_STATE_IDLE || on_done == NULL ||
        (mode != PLATFORM_LINE_PUSH_PULL && mode != PLATFORM_LINE_OPEN_DRAIN))
    {
        return false;
    }
    line_mode = mode;
    done_callback = on_done;
    rise_us = 0;
    enter(EDGE_STATE_DISCHARGE, false);
    return true;
}

void edge_task_handle(void)
{
    if (state == EDGE_STATE_IDLE)
    {
        return;
    }
    float voltage = platform_voltage_read();
    uint64_t elapsed_us = systime_us() - phase_start_us;
    if (elapsed_us >= TIMEOUT_US)
    {
        finish(false, 0);
        return;
    }
    switch (state)
    {
    case EDGE_STATE_IDLE:
        break;
    case EDGE_STATE_DISCHARGE:
        if (voltage < EMPTY_V)
        {
            enter(EDGE_STATE_RISE, true);
        }
        break;
    case EDGE_STATE_RISE:
        if (voltage > RISE_V)
        {
            rise_us = (uint32_t)elapsed_us;
            enter(EDGE_STATE_CHARGE, true);
        }
        break;
    case EDGE_STATE_CHARGE:
        if (voltage > FULL_V)
        {
            enter(EDGE_STATE_FALL, false);
        }
        break;
    case EDGE_STATE_FALL:
        if (voltage < FALL_V)
        {
            finish(true, (uint32_t)elapsed_us);
        }
        break;
    }
}
