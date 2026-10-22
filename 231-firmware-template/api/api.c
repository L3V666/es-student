#include "api.h"
#include "device.h"
#include "profiling.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef void (*api_callback_t)(const command_t *command);

typedef struct
{
    const char *name;
    const char *help;
    api_callback_t callback;
} api_command_t;

static bool parse_u32(const char *text, uint32_t *value)
{
    if (!isdigit((unsigned char)text[0]))
    {
        return false;
    }
    char *end;
    errno = 0;
    unsigned long number = strtoul(text, &end, 10);
    if (*end != '\0' || errno == ERANGE || number > UINT32_MAX)
    {
        return false;
    }
    *value = (uint32_t)number;
    return true;
}

static void cmd_info(const command_t *command)
{
    (void)command;
    device_info();
}

static void cmd_uptime(const command_t *command)
{
    (void)command;
    printf("uptime: %llu ms\n", (unsigned long long)(time_us_64() / 1000));
}

static double calc_pi(uint32_t terms)
{
    double sum = 0.0;
    double sign = 1.0;
    for (uint32_t k = 0; k < terms; k++)
    {
        sum += sign / (2.0 * k + 1.0);
        sign = -sign;
    }
    return 4.0 * sum;
}

static void cmd_calc_pi(const command_t *command)
{
    uint32_t terms = 1000000;
    if (command->argc > 1 ||
        (command->argc == 1 && !parse_u32(command->argv[0], &terms)))
    {
        printf("error: usage calc_pi [terms]\n");
        return;
    }
    if (terms == 0)
    {
        printf("error: terms must be greater than 0\n");
        return;
    }
    uint64_t start_us = time_us_64();
    volatile double pi_result = calc_pi(terms);
    uint64_t spent_us = time_us_64() - start_us;
    printf("pi: %.9f\n", pi_result);
    printf("time: %llu ms\n", (unsigned long long)(spent_us / 1000));
}

static void cmd_main_time_exec(const command_t *command)
{
    (void)command;
    printf("iteration avg %.2f us, max %u us\n",
           profiling_avg_us(), (unsigned)profiling_max_us());
}

static void cmd_main_time_reset(const command_t *command)
{
    (void)command;
    profiling_reset_max();
    printf("max reset\n");
}

static const api_command_t api_commands[] = {
    {"info", "device passport", cmd_info},
    {"uptime", "time since reset", cmd_uptime},
    {"calc_pi", "calculate pi: calc_pi [terms]", cmd_calc_pi},
    {"main_time_exec", "superloop iteration: average and maximum", cmd_main_time_exec},
    {"main_time_reset", "reset maximum iteration", cmd_main_time_reset},
};

void api_handle(const command_t *command)
{
    if (command->truncated)
    {
        printf("error: command longer than 63 characters or 4 arguments\n");
        return;
    }
    if (strcmp(command->name, "help") == 0)
    {
        printf("%-16s %s\n", "help", "list of commands");
        for (uint32_t i = 0; i < sizeof(api_commands) / sizeof(api_commands[0]); i++)
        {
            printf("%-16s %s\n", api_commands[i].name, api_commands[i].help);
        }
        return;
    }
    for (uint32_t i = 0; i < sizeof(api_commands) / sizeof(api_commands[0]); i++)
    {
        if (strcmp(command->name, api_commands[i].name) == 0)
        {
            api_commands[i].callback(command);
            return;
        }
    }
    printf("error: unknown command '%s', try help\n", command->name);
}
