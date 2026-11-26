#include "api/api-commands.h"
#include "systime/systime.h"
#include "device/device.h"
#include "firmware.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "profiling/profiling.h"
#include "pi-task/pi-task.h"
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "edge-task/edge-task.h"
#include "uart-task/uart-task.h"
#include "block-task/block-task.h"

#define ECHO_TEXT_MAX 32

static bool scope_active;
#define UART_BASE_BAUD 115200

static const char text_line[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

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
    device_info_t info;
    device_get_info(&info);
    printf("name: %s\n", FIRMWARE_NAME);
    printf("version: %s\n", FIRMWARE_VERSION);
    printf("project: %s\n", FIRMWARE_PROJECT);
    printf("repo: %s\n", FIRMWARE_REPO);
    printf("board: %s\n", info.board);
    printf("serial: %s\n", info.serial);
    printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n",
           (unsigned)info.manufacturer, (unsigned)info.part, (unsigned)info.revision);
    printf("pico-sdk: %s\n", info.sdk_version);
}

static void cmd_uptime(const command_t *command)
{
    (void)command;
    printf("uptime: %llu ms\n", (unsigned long long)(systime_ms()));
}

static void cmd_pi_start(const command_t *command)
{
    uint32_t terms = 1000000;
    if (command->argc > 1 ||
        (command->argc == 1 && !parse_u32(command->argv[0], &terms)))
    {
        printf("error: usage pi_start [<terms>]\n");
        return;
    }
    if (terms == 0)
    {
        printf("error: terms must be greater than 0\n");
        return;
    }
    if (!pi_task_start(terms))
    {
        printf("error: pi is busy\n");
        return;
    }
    printf("pi: started, %u terms\n", (unsigned)terms);
}

static void cmd_pi(const command_t *command)
{
    (void)command;
    double result;
    uint64_t elapsed_ms;
    if (pi_task_get_result(&result, &elapsed_ms))
    {
        printf("pi: %.9f in %llu ms\n", result, (unsigned long long)elapsed_ms);
    }
    else if (pi_task_get_state() == PI_STATE_RUNNING)
    {
        printf("pi: running\n");
    }
    else
    {
        printf("pi: not started\n");
    }
}

static void cmd_profiling(const command_t *command)
{
    (void)command;
#if PROFILING_ENABLED
    for (uint32_t id = 0; id < PROFILING_STOPWATCH_COUNT; id++)
    {
        profiling_result_t result;
        if (profiling_get(id, &result))
        {
            printf("%-10s count %-10u mean %.2f us  max %u us\n",
                   result.name, (unsigned)result.count, result.mean_us,
                   (unsigned)result.max_us);
        }
    }
#else
    printf("profiling is disabled in this build\n");
#endif
}

static void cmd_profiling_reset(const command_t *command)
{
    (void)command;
    profiling_reset();
    printf("profiling reset\n");
}

static void print_led_state(void)
{
    const char *state = "off";
    switch (led_task_get_state())
    {
    case LED_STATE_OFF:
        state = "off";
        break;
    case LED_STATE_ON:
        state = "on";
        break;
    case LED_STATE_BLINK:
        state = "blink";
        break;
    }
    printf("led: %s, period %u ms\n", state, (unsigned)led_task_get_period_ms());
}

static void cmd_led_enable(const command_t *command)
{
    (void)command;
    led_task_set_state(LED_STATE_ON);
    print_led_state();
}

static void cmd_led_disable(const command_t *command)
{
    (void)command;
    led_task_set_state(LED_STATE_OFF);
    print_led_state();
}

static void cmd_led_blink(const command_t *command)
{
    (void)command;
    led_task_set_state(LED_STATE_BLINK);
    print_led_state();
}

static void cmd_led_period(const command_t *command)
{
    uint32_t period_ms;
    if (command->argc != 1 || !parse_u32(command->argv[0], &period_ms))
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }
    if (!led_task_set_period_ms(period_ms))
    {
        printf("error: period_ms must be greater than 0\n");
        return;
    }
    print_led_state();
}

static void cmd_button(const command_t *command)
{
    (void)command;
    printf("button: %s, presses %u\n",
           button_task_is_pressed() ? "pressed" : "released",
           (unsigned)button_task_get_press_count());
}

static void print_edge(platform_line_mode_t mode, bool ok, uint32_t rise_us, uint32_t fall_us)
{
    const char *name = mode == PLATFORM_LINE_PUSH_PULL ? "pp" : "od";
    if (!ok)
    {
        printf("edge %s: timeout\n", name);
        return;
    }
    printf("edge %s: rise %.1f ms, fall %.1f ms\n", name,
           rise_us / 1000.0, fall_us / 1000.0);
}

static void cmd_edge(const command_t *command)
{
    platform_line_mode_t mode;
    if (command->argc == 1 && strcmp(command->argv[0], "pp") == 0)
    {
        mode = PLATFORM_LINE_PUSH_PULL;
    }
    else if (command->argc == 1 && strcmp(command->argv[0], "od") == 0)
    {
        mode = PLATFORM_LINE_OPEN_DRAIN;
    }
    else
    {
        printf("error: usage edge pp|od\n");
        return;
    }
    if (scope_active)
    {
        printf("error: adc busy\n");
        return;
    }
    if (!edge_task_start(mode, print_edge))
    {
        printf("error: edge is busy\n");
    }
}

static void print_bytes(const uint8_t *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        putchar(isprint((unsigned char)data[i]) ? data[i] : '.');
    }
}

static void print_errors(uint32_t errors)
{
    printf("err:");
    if (errors == 0)
    {
        printf(" none");
    }
    if (errors & PLATFORM_UART_FE) printf(" FE");
    if (errors & PLATFORM_UART_PE) printf(" PE");
    if (errors & PLATFORM_UART_BE) printf(" BE");
    if (errors & PLATFORM_UART_OE) printf(" OE");
}

static void print_echo(const uint8_t *data, size_t size, uint32_t errors)
{
    printf("rx: ");
    print_bytes(data, size);
    printf(" ");
    print_errors(errors);
    printf("\n");
}

static void cmd_loop(const command_t *command)
{
    bool internal;
    if (command->argc == 1 && strcmp(command->argv[0], "internal") == 0)
    {
        internal = true;
    }
    else if (command->argc == 1 && strcmp(command->argv[0], "pins") == 0)
    {
        internal = false;
    }
    else
    {
        printf("error: usage loop internal|pins\n");
        return;
    }
    if (scope_active || uart_task_busy())
    {
        printf("error: uart is busy\n");
        return;
    }
    platform_uart_loop_set(internal);
    printf("loop: %s\n", command->argv[0]);
}

// echo и link принимают один аргумент, помещающийся в очередь UART.
static bool text_argument(const command_t *command, const char *name, size_t *size)
{
    if (command->argc != 1)
    {
        printf("error: usage %s <text>\n", name);
        return false;
    }
    *size = strlen(command->argv[0]);
    if (*size > ECHO_TEXT_MAX)
    {
        printf("error: text longer than 32\n");
        return false;
    }
    return true;
}

static void cmd_echo(const command_t *command)
{
    size_t size;
    if (!text_argument(command, "echo", &size))
    {
        return;
    }
    if (scope_active || !uart_task_start(PLATFORM_UART0, PLATFORM_UART0,
                         (const uint8_t *)command->argv[0], size, print_echo))
    {
        printf("error: uart is busy\n");
    }
}

static void print_link_back(const uint8_t *data, size_t size, uint32_t errors)
{
    printf("1->0: ");
    print_bytes(data, size);
    printf(" ");
    print_errors(errors);
    printf("\n");
}

static void print_link_forth(const uint8_t *data, size_t size, uint32_t errors)
{
    printf("0->1: ");
    print_bytes(data, size);
    printf(" ");
    print_errors(errors);
    printf("\n");
    // Задача уже IDLE; она копирует принятые байты до выхода из callback.
    if (!uart_task_start(PLATFORM_UART1, PLATFORM_UART0, data, size, print_link_back))
    {
        printf("error: uart is busy\n");
    }
}

static void cmd_link(const command_t *command)
{
    size_t size;
    if (!text_argument(command, "link", &size))
    {
        return;
    }
    if (scope_active || !uart_task_start(PLATFORM_UART0, PLATFORM_UART1,
                         (const uint8_t *)command->argv[0], size, print_link_forth))
    {
        printf("error: uart is busy\n");
    }
}

static void cmd_baud_shift(const command_t *command)
{
    if (command->argc != 1)
    {
        printf("error: usage baud_shift <percent>\n");
        return;
    }
    const char *text = command->argv[0];
    const char *digits = text;
    if (*digits == '+' || *digits == '-') digits++;
    if (!isdigit((unsigned char)*digits))
    {
        printf("error: usage baud_shift <percent>\n");
        return;
    }
    char *end;
    errno = 0;
    long percent = strtol(text, &end, 10);
    if (*end != '\0')
    {
        printf("error: usage baud_shift <percent>\n");
        return;
    }
    if (errno == ERANGE || percent < -50 || percent > 50)
    {
        printf("error: percent must be -50..50\n");
        return;
    }
    if (scope_active || uart_task_busy())
    {
        printf("error: uart is busy\n");
        return;
    }
    uint32_t baud = UART_BASE_BAUD * (100 + percent) / 100;
    uint32_t actual = platform_uart_baud_set(PLATFORM_UART1, baud);
    printf("baud_shift: uart1 %u baud\n", (unsigned)actual);
}

static void print_text(const uint8_t *data, size_t size, uint32_t errors)
{
    printf("text: %u chars ", (unsigned)size);
    print_bytes(data, size);
    printf(" ");
    print_errors(errors);
    printf("\n");
}

static void cmd_text(const command_t *command)
{
    if (command->argc != 0)
    {
        printf("error: usage text\n");
        return;
    }
    if (scope_active || !uart_task_start(PLATFORM_UART1, PLATFORM_UART0,
                         (const uint8_t *)text_line, sizeof(text_line) - 1, print_text))
    {
        printf("error: uart is busy\n");
    }
}

static bool parse_parity(const char *text, platform_parity_t *parity)
{
    if (strcmp(text, "none") == 0) *parity = PLATFORM_PARITY_NONE;
    else if (strcmp(text, "even") == 0) *parity = PLATFORM_PARITY_EVEN;
    else if (strcmp(text, "odd") == 0) *parity = PLATFORM_PARITY_ODD;
    else return false;
    return true;
}

static void cmd_parity(const command_t *command)
{
    platform_parity_t rx, tx;
    if (command->argc != 2 || !parse_parity(command->argv[0], &rx) ||
        !parse_parity(command->argv[1], &tx))
    {
        printf("error: usage parity none|even|odd none|even|odd\n");
        return;
    }
    if (scope_active || uart_task_busy())
    {
        printf("error: uart is busy\n");
        return;
    }
    platform_uart_parity_set(PLATFORM_UART0, rx);
    platform_uart_parity_set(PLATFORM_UART1, tx);
    printf("parity: uart0 %s, uart1 %s\n", command->argv[0], command->argv[1]);
}

#define BLOCK_SIZE 64
static uint8_t block_data[BLOCK_SIZE];
static bool block_irq;

static void print_block(const uint8_t *data, size_t size, uint32_t errors)
{
    platform_uart_rx_irq_set(false);
    bool match = size > 0 && size <= BLOCK_SIZE && memcmp(data, block_data, size) == 0;
    printf("block %s: %u/64 %s, ", block_irq ? "irq" : "poll",
           (unsigned)size, match ? "match" : "mismatch");
    print_errors(errors);
    printf("\n");
}

static void cmd_block(const command_t *command)
{
    bool use_irq;
    if (command->argc == 1 && strcmp(command->argv[0], "irq") == 0)
    {
        use_irq = true;
    }
    else if (command->argc == 1 && strcmp(command->argv[0], "poll") == 0)
    {
        use_irq = false;
    }
    else
    {
        printf("error: usage block irq|poll\n");
        return;
    }
    // Повторная команда не должна менять режим уже идущего обмена.
    if (scope_active || uart_task_busy())
    {
        printf("error: uart is busy\n");
        return;
    }
    for (size_t i = 0; i < BLOCK_SIZE; i++)
    {
        block_data[i] = (uint8_t)i;
    }
    block_irq = use_irq;
    platform_uart_rx_irq_set(use_irq);
    if (!uart_task_start(PLATFORM_UART1, PLATFORM_UART0,
                         block_data, sizeof(block_data), print_block))
    {
        platform_uart_rx_irq_set(false);
        printf("error: uart is busy\n");
    }
}

#define SCOPE_SAMPLES PLATFORM_BLOCK_MAX
#define SCOPE_BAUD_MIN 9600
#define SCOPE_TEXT_MAX 4

static uint32_t scope_baud;
static uint8_t scope_rx[UART_TASK_MAX];
static size_t scope_rx_size;
static uint32_t scope_rx_errors;
static bool scope_rx_done;
static bool scope_failed;
static const uint16_t *scope_samples;
static uint32_t scope_count;

static void finish_scope(void)
{
    if (scope_samples == NULL || !scope_rx_done) return;
    platform_uart_baud_set(PLATFORM_UART0, UART_BASE_BAUD);
    platform_uart_baud_set(PLATFORM_UART1, UART_BASE_BAUD);
    if (!scope_failed)
    {
        printf("scope: %u samples, %u Hz, %u baud, uart1 ",
               (unsigned)scope_count, (unsigned)PLATFORM_SAMPLE_RATE_HZ,
               (unsigned)scope_baud);
        print_bytes(scope_rx, scope_rx_size);
        printf(" ");
        print_errors(scope_rx_errors);
        printf("\n");
        for (uint32_t i = 0; i < scope_count; i++)
        {
            printf("%u%c", (unsigned)scope_samples[i],
                   ((i + 1) % 16 == 0 || i + 1 == scope_count) ? '\n' : ' ');
        }
        printf("scope: end\n");
    }
    scope_samples = NULL;
    scope_active = false;
}

static void print_scope_rx(const uint8_t *data, size_t size, uint32_t errors)
{
    memcpy(scope_rx, data, size);
    scope_rx_size = size;
    scope_rx_errors = errors;
    scope_rx_done = true;
    finish_scope();
}

static void print_scope(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    (void)time_us;
    scope_samples = samples;
    scope_count = count;
    // Обычно UART заканчивает первым; при задержке суперцикла ждём его callback.
    finish_scope();
}

static void cmd_scope(const command_t *command)
{
    uint32_t baud;
    if (command->argc != 2 || !parse_u32(command->argv[0], &baud) ||
        baud < SCOPE_BAUD_MIN || baud > UART_BASE_BAUD ||
        strlen(command->argv[1]) == 0 || strlen(command->argv[1]) > SCOPE_TEXT_MAX)
    {
        printf("error: usage scope <baud> <text>\n");
        return;
    }
    if (scope_active || edge_task_busy())
    {
        printf("error: adc busy\n");
        return;
    }
    if (uart_task_busy())
    {
        printf("error: uart is busy\n");
        return;
    }
    platform_uart_baud_set(PLATFORM_UART0, baud);
    platform_uart_baud_set(PLATFORM_UART1, baud);
    scope_baud = baud;
    scope_rx_size = 0;
    scope_rx_errors = 0;
    scope_rx_done = false;
    scope_failed = false;
    scope_samples = NULL;
    scope_active = true;
    if (!block_task_start(SCOPE_SAMPLES, print_scope))
    {
        scope_active = false;
        platform_uart_baud_set(PLATFORM_UART0, UART_BASE_BAUD);
        platform_uart_baud_set(PLATFORM_UART1, UART_BASE_BAUD);
        printf("error: adc busy\n");
        return;
    }
    if (!uart_task_start(PLATFORM_UART0, PLATFORM_UART1,
                         (const uint8_t *)command->argv[1], strlen(command->argv[1]),
                         print_scope_rx))
    {
        // DMA завершится самостоятельно; его callback восстановит скорость.
        scope_failed = true;
        scope_rx_done = true;
        printf("error: uart is busy\n");
    }
}

const api_command_t api_commands[] = {
    {"scope", "UART0 line on ADC: scope <baud> <text>", cmd_scope},
    {"block", "64 bytes from UART1 to UART0: block irq|poll", cmd_block},
    {"link", "text from UART0 to UART1 and back: link <text>", cmd_link},
    {"baud_shift", "shift UART1 baud rate: baud_shift <percent>", cmd_baud_shift},
    {"text", "32 characters from UART1 to UART0", cmd_text},
    {"parity", "parity of UART0 and UART1: parity <rx> <tx>", cmd_parity},
    {"loop", "UART0 loopback: loop internal|pins", cmd_loop},
    {"echo", "text through UART0 and back: echo <text>", cmd_echo},
    {"edge", "rise and fall of the RC line: edge pp|od", cmd_edge},
    {"button", "button state and press count", cmd_button},
    {"led_enable", "turn led on", cmd_led_enable},
    {"led_disable", "turn led off", cmd_led_disable},
    {"led_blink", "blink led", cmd_led_blink},
    {"led_period", "set blink period: led_period <period_ms>", cmd_led_period},
    {"info", "device passport", cmd_info},
    {"uptime", "time since reset", cmd_uptime},
    {"pi_start", "start pi calculation: pi_start [<terms>]", cmd_pi_start},
    {"pi", "pi result or calculation state", cmd_pi},
    {"profiling", "stopwatch counts, means and maxima", cmd_profiling},
    {"profiling_reset", "reset all stopwatch results", cmd_profiling_reset},
};

const uint32_t api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);
