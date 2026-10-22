#include "profiling.h"
#include "pico/stdlib.h"

typedef struct
{
    const char *name;
    uint32_t start_us;
    uint32_t count;
    uint32_t sum_us;
    uint32_t max_us;
    bool running;
} stopwatch_t;

static stopwatch_t stopwatches[PROFILING_STOPWATCH_COUNT];

void profiling_stopwatch_init(uint32_t id, const char *name)
{
    if (id >= PROFILING_STOPWATCH_COUNT)
    {
        return;
    }
    stopwatches[id] = (stopwatch_t){.name = name};
}

void profiling_start(uint32_t id)
{
    if (id >= PROFILING_STOPWATCH_COUNT || stopwatches[id].name == NULL)
    {
        return;
    }
    stopwatches[id].start_us = time_us_32();
    stopwatches[id].running = true;
}

void profiling_stop(uint32_t id)
{
    if (id >= PROFILING_STOPWATCH_COUNT || !stopwatches[id].running)
    {
        return;
    }
    stopwatch_t *watch = &stopwatches[id];
    uint32_t elapsed_us = time_us_32() - watch->start_us;
    watch->running = false;
    watch->count++;
    watch->sum_us += elapsed_us;
    if (elapsed_us > watch->max_us)
    {
        watch->max_us = elapsed_us;
    }
}

bool profiling_get(uint32_t id, profiling_result_t *result)
{
    if (id >= PROFILING_STOPWATCH_COUNT || stopwatches[id].name == NULL)
    {
        return false;
    }
    const stopwatch_t *watch = &stopwatches[id];
    result->name = watch->name;
    result->count = watch->count;
    result->mean_us = watch->count == 0 ? 0.0 : (double)watch->sum_us / watch->count;
    result->max_us = watch->max_us;
    return true;
}

void profiling_reset(void)
{
    for (uint32_t id = 0; id < PROFILING_STOPWATCH_COUNT; id++)
    {
        stopwatches[id] = (stopwatch_t){.name = stopwatches[id].name};
    }
}
