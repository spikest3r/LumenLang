/*
 * Lumen VM for RP2040 (Raspberry Pi Pico).
 *
 * Source layout:
 *   main.c       - hardware init, UART/stdio I/O, entry point
 *   vm.h         - shared types and declarations
 *   vm.c         - instruction dispatch (all desktop opcodes)
 *   natives.c    - native functions (print, input, string, random, GPIO, ...)
 *   vm_memory.c  - GC'd string heap + arrays
 *   loader.c     - program image parser
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/timer.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define UART_ID uart1
#define BAUD_RATE 115200
#define UART_TX_PIN 4
#define UART_RX_PIN 5

#include "program.h"
#include "vm.h"

static LumenProgram g_program;
static VM g_vm;                 /* ~5 KB: keep off the (small) C stack */

void send_uart(const char* message) {
    uart_puts(UART_ID, message);
    printf("%s", message);
}

void uart_readline(char* buffer, int maxLen) {
    int i = 0;

    while(i < maxLen - 1) {
        int c = getchar();

        if(c == '\r' || c == '\n')
            break;

        buffer[i++] = (char)c;
        putchar(c);
    }

    buffer[i] = '\0';
    printf("\n");
}

int main() {
    stdio_init_all();
    sleep_ms(2000);

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    srand(time_us_32());

    send_uart("Loading...\n");

    if(loadFromFlash(&g_program, program, programSize) != 0) {
        send_uart("Load failed\n");
        while(1);
    }

    send_uart("Executing...\n");
    vm_init(&g_vm, &g_program);
    vm_run(&g_vm);
    send_uart("Done\n");

    while(1);
}
