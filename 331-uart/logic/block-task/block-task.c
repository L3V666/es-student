#include "block-task/block-task.h"
#include "platform/platform.h"
#include <stddef.h>

static block_task_callback_t block_callback;
static uint32_t block_count;

bool block_task_start(uint32_t count, block_task_callback_t on_block)
{
    if (block_callback != NULL || on_block == NULL || !platform_block_start(count))
    {
        return false;
    }
    block_count = count;
    block_callback = on_block;
    return true;
}

void block_task_handle(void)
{
    if (block_callback == NULL || !platform_block_done())
    {
        return;
    }
    block_task_callback_t callback = block_callback;
    uint32_t count = block_count;
    block_callback = NULL;
    callback(platform_block_samples(), count, platform_block_time_us());
}
