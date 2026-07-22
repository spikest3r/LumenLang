#include "uart.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define BAUD 9600
#define UBRR_VALUE ((F_CPU / 16 / BAUD) - 1)

void uart_init() {
  UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
  UBRR0L = (uint8_t)UBRR_VALUE;

  UCSR0B = (1 << TXEN0) | (1 << RXEN0);

  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_putchar(char c) {
  while (!(UCSR0A & (1 << UDRE0)));
  UDR0 = c;
}

void uart_puts(const char* s) {
  while (*s) {
    uart_putchar(*s++);
  }
}

char uart_getchar(void) {
  while (!(UCSR0A & (1 << RXC0)));
  return UDR0;
}

void send_uart(const char* message) {
  uart_puts(message);
  printf("%s", message);
}

void uart_put_float(float value) {
    char buf[32];

    int32_t whole = (int32_t)value;
    int32_t frac = (int32_t)((value - whole) * 100);

    if (frac < 0)
        frac = -frac;

    sprintf(buf, "%ld.%02ld", (long)whole, (long)frac);

    send_uart(buf);
}


void uart_put_variant(const Variant* v) {
    char buf[32];

    if (v->type == TAG_STRING) {
        send_uart(v->data.str);
        return;
    }
    else if (v->type == TAG_FLOAT) {
        uart_put_float(v->data.f);
        return;
    }
    else {
        snprintf(buf, sizeof(buf), "%ld", (long)v->data.i);
        send_uart(buf);
    }
}

void uart_readline(char* buffer, int maxLen) {
  int i = 0;

  while (i < maxLen - 1) {
    char c = uart_getchar();

    if (c == '\r' || c == '\n')
      break;

    buffer[i++] = c;
    uart_putchar(c);
  }

  buffer[i] = '\0';

  uart_putchar('\r');
  uart_putchar('\n');
}