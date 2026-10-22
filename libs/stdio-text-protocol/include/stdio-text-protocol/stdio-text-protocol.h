#pragma once

#include <stdbool.h>
#include <stdint.h>

#define STDIO_TEXT_PROTOCOL_LINE_SIZE 64
#define STDIO_TEXT_PROTOCOL_MAX_ARGS 4

// Команда, принятая из одной строки.
typedef struct
{
    const char *name;
    uint32_t argc;
    const char *argv[STDIO_TEXT_PROTOCOL_MAX_ARGS];
    // Строка или список аргументов не поместились: выполнять команду нельзя.
    bool truncated;
} command_t;

void stdio_text_protocol_init(void);

// Читает один символ без ожидания. Возвращает команду по концу строки,
// иначе NULL. Команда действительна до следующего вызова.
const command_t *stdio_text_protocol_handle(void);
