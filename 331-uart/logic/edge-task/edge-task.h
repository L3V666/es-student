#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "platform/platform.h"

// Времена в микросекундах; ok == false — порог не достигнут за 2 с.
typedef void (*edge_task_callback_t)(platform_line_mode_t mode, bool ok,
                                     uint32_t rise_us, uint32_t fall_us);
bool edge_task_start(platform_line_mode_t mode, edge_task_callback_t on_done);
void edge_task_handle(void);
