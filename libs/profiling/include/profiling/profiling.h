#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PROFILING_STOPWATCH_COUNT 5

#ifndef PROFILING_ENABLED
#define PROFILING_ENABLED 1
#endif

// Сумма хранится в uint32_t и переполняется после примерно 71 минуты
// накопленного времени секундомера. Сбрасывайте результаты до этого;
// число замеров также ограничено UINT32_MAX.
typedef struct
{
    const char *name;
    uint32_t count;
    double mean_us;
    uint32_t max_us;
} profiling_result_t;

// Имя должно оставаться действительным всё время работы секундомера.
void profiling_stopwatch_init(uint32_t id, const char *name);
#if PROFILING_ENABLED
void profiling_start(uint32_t id);
void profiling_stop(uint32_t id);
#else
#define profiling_start(id) ((void)0)
#define profiling_stop(id) ((void)0)
#endif

// false — секундомер с таким номером не заведён.
bool profiling_get(uint32_t id, profiling_result_t *result);
// Сохраняет имена, обнуляет статистику и отменяет незавершённые замеры.
void profiling_reset(void);
