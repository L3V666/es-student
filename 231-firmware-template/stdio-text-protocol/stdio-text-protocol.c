#include "stdio-text-protocol.h"
#include "pico/stdlib.h"
#include <stdio.h>

static char line[STDIO_TEXT_PROTOCOL_LINE_SIZE];
static uint32_t line_length;
static bool overflow;
static command_t command;

void stdio_text_protocol_init(void)
{
    line_length = 0;
    overflow = false;
    line[0] = '\0';
    command = (command_t){0};
}

static const command_t *parse_line(void)
{
    command = (command_t){.truncated = overflow};
    char *cursor = line;
    while (*cursor != '\0')
    {
        if (*cursor == ' ')
        {
            *cursor++ = '\0';
            continue;
        }

        char *word = cursor;
        while (*cursor != '\0' && *cursor != ' ')
        {
            cursor++;
        }
        if (*cursor == ' ')
        {
            *cursor++ = '\0';
        }

        if (command.name == NULL)
        {
            command.name = word;
        }
        else if (command.argc < STDIO_TEXT_PROTOCOL_MAX_ARGS)
        {
            command.argv[command.argc++] = word;
        }
        else
        {
            command.truncated = true;
        }
    }

    // Даже если сохранены только пробелы, переполненный кадр надо отклонить.
    return command.name != NULL || command.truncated ? &command : NULL;
}

const command_t *stdio_text_protocol_handle(void)
{
    int symbol = getchar_timeout_us(0);
    if (symbol == PICO_ERROR_TIMEOUT)
    {
        return NULL;
    }

    if (symbol == '\r' || symbol == '\n')
    {
        if (line_length == 0)
        {
            return NULL;
        }
        putchar('\n'); // Pico SDK преобразует LF в CR LF.
        line[line_length] = '\0';
        const command_t *result = parse_line();
        line_length = 0;
        overflow = false;
        return result;
    }

    if (symbol == 0x08 || symbol == 0x7f)
    {
        if (line_length > 0)
        {
            line_length--;
            overflow = false;
            printf("\b \b");
        }
        return NULL;
    }

    if (symbol >= 0x20 && symbol <= 0x7e)
    {
        if (line_length < STDIO_TEXT_PROTOCOL_LINE_SIZE - 1)
        {
            line[line_length++] = (char)symbol;
            putchar(symbol);
        }
        else
        {
            overflow = true;
        }
    }
    return NULL;
}
