#pragma once

#include "includes.h"

void uart_init(void);
void uart_putchar(char c);
void uart_puts(const char* s);
char uart_getchar(void);
void send_uart(const char* message);
void uart_put_variant(const Variant* v);
void uart_readline(char* buffer, int maxLen);