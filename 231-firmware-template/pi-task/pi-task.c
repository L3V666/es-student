#include "pi-task.h"
#include "pico/stdlib.h"

// Один член: около 585 тактов / 125 МГц = 4,68 мкс.
// 100 членов: около 468 мкс, меньше 0,5 мс; проверить на плате.
#define PI_TERMS_PER_CALL 100

static pi_state_t pi_state = PI_STATE_NOT_STARTED;
static uint32_t pi_k;
static uint32_t pi_n;
static double pi_sum;
static double pi_sign;
static double pi_result;
static uint64_t start_us;
static uint64_t finish_us;

bool pi_task_start(uint32_t terms)
{
    if (pi_state == PI_STATE_RUNNING || terms == 0)
    {
        return false;
    }
    pi_n = terms;
    pi_k = 0;
    pi_sum = 0.0;
    pi_sign = 1.0;
    start_us = time_us_64();
    pi_state = PI_STATE_RUNNING;
    return true;
}

void pi_task_handle(void)
{
    if (pi_state != PI_STATE_RUNNING)
    {
        return;
    }

    uint32_t end = pi_k + PI_TERMS_PER_CALL;
    // При переполнении или выходе за число членов ограничиваем порцию.
    if (end < pi_k || end > pi_n)
    {
        end = pi_n;
    }
    for (; pi_k < end; pi_k++)
    {
        pi_sum += pi_sign / (2.0 * pi_k + 1.0);
        pi_sign = -pi_sign;
    }

    if (pi_k == pi_n)
    {
        pi_result = 4.0 * pi_sum;
        finish_us = time_us_64();
        pi_state = PI_STATE_READY;
    }
}

pi_state_t pi_task_get_state(void)
{
    return pi_state;
}

bool pi_task_get_result(double *result, uint64_t *elapsed_ms)
{
    if (pi_state != PI_STATE_READY)
    {
        return false;
    }
    *result = pi_result;
    *elapsed_ms = (finish_us - start_us) / 1000;
    return true;
}
