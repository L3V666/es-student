#include "stream-task/stream-task.h"
#include "platform/platform.h"
#include <stddef.h>

_Static_assert(sizeof(stream_header_t) == 6, "Stream header must contain 6 bytes");

static stream_state_t stream_state;
static stream_task_callback_t frame_callback;
static uint8_t stream_bits;
static uint16_t frame_number;
static uint32_t frame_count;
static uint32_t sample_count;

void stream_task_init(stream_task_callback_t on_frame)
{
    frame_callback = on_frame;
    stream_state = STREAM_STATE_IDLE;
    stream_bits = 16;
    frame_number = 0;
    frame_count = 0;
    sample_count = 0;
}

bool stream_task_start(uint8_t bits)
{
    if (stream_state != STREAM_STATE_IDLE || frame_callback == NULL ||
        !platform_stream_start(bits))
    {
        return false;
    }
    stream_bits = bits;
    frame_number = 0;
    frame_count = 0;
    sample_count = 0;
    stream_state = STREAM_STATE_RUN;
    return true;
}

void stream_task_stop(void)
{
    if (stream_state == STREAM_STATE_RUN)
    {
        platform_stream_stop();
        stream_state = STREAM_STATE_IDLE;
    }
}

void stream_task_handle(void)
{
    if (stream_state != STREAM_STATE_RUN)
    {
        return;
    }
    uint32_t samples;
    const uint8_t *data = platform_stream_next(&samples);
    if (data == NULL)
    {
        return;
    }
    stream_header_t header = {
        .sync_word = STREAM_SYNC_WORD,
        .number = frame_number,
        .count = (uint16_t)samples,
    };
    uint32_t bytes = samples * (stream_bits / 8);
    frame_callback(&header, data, bytes);
    platform_stream_release();
    frame_number++;
    frame_count++;
    sample_count += samples;
}

stream_state_t stream_task_get_state(void)
{
    return stream_state;
}

uint8_t stream_task_get_bits(void)
{
    return stream_bits;
}

uint32_t stream_task_get_frames(void)
{
    return frame_count;
}

uint32_t stream_task_get_samples(void)
{
    return sample_count;
}

uint32_t stream_task_get_overruns(void)
{
    return platform_stream_overruns();
}
