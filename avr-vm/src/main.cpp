#include "includes.h"
#include "program.h"
#include "vm.h"
#include "vmfuncmap.h"
#include "uart.h"
#include "vmnativefuncmap.h"
#include "helpers.h"

static uint8_t* g_bytecode = NULL;
static int g_bytecodeSize = 0;

static char* g_stringPool[MAX_STRINGS];
static int g_stringPoolSize = 0;

static float g_constPool[MAX_CONST_POOL];
static int g_constPoolSize = 0;

static int g_variableCount = 0;

int loadFromFlash(const uint8_t* data, int dataSize) {
    int pos = 0;

    uint8_t sig0 = readByte(data, &pos);
    uint8_t sig1 = readByte(data, &pos);

    bool isV2 = (sig0 == 0xFE && sig1 == 0xFB);
    bool isV3 = (sig0 == 0xFE && sig1 == 0xFC);
    bool isV4 = (sig0 == 0xFE && sig1 == 0xFD);
    bool isV5 = (sig0 == 0xFE && sig1 == 0xFE);
    bool isV6 = (sig0 == 0xFE && sig1 == 0xFF);

    if (!isV2 && !isV3 && !isV4 && !isV5 && !isV6) {
        send_uart("Invalid signature\n");

        if (sig0 == 0xFE && sig1 == 0xFA) {
            send_uart("v1 Precompiled Lumen binaries are not compatible with v2+ Lumen runtime\n");
        }

        return -1;
    }

    int bcSize = readInt(data, &pos);

    if (g_bytecode)
        free(g_bytecode);

    g_bytecode = (uint8_t*)malloc(bcSize);

    if (!g_bytecode) {
        send_uart("malloc failed\n");
        return -1;
    }

    g_bytecodeSize = bcSize;

    for (int i = 0; i < bcSize; i++) {
        g_bytecode[i] = readByte(data, &pos);
    }

    int spSize = readInt(data, &pos);

    for (int i = 0; i < g_stringPoolSize; i++) {
        if (g_stringPool[i]) {
            free(g_stringPool[i]);
            g_stringPool[i] = NULL;
        }
    }

    g_stringPoolSize = spSize;

    for (int i = 0; i < spSize; i++) {
        int len = readInt(data, &pos);

        g_stringPool[i] = (char*)malloc(len + 1);

        if (!g_stringPool[i]) {
            send_uart("string pool error");
            return -1;
        }

        for (int j = 0; j < len; j++) {
            g_stringPool[i][j] = (char)readByte(data, &pos);
        }

        g_stringPool[i][len] = '\0';
    }

    int cpSize = readInt(data, &pos);

    g_constPoolSize = cpSize;

    if (!isV2) {
        for (int i = 0; i < cpSize; i++) {
            g_constPool[i] = readfloat(data, &pos);
        }
    } else {
        for (int i = 0; i < cpSize; i++) {
            g_constPool[i] = (float)readInt(data, &pos);
        }
    }

    g_variableCount = readInt(data, &pos);

    return 0;
}

int main() {
  _delay_ms(1000);
  uart_init();

  memset(g_stringPool, 0, sizeof(g_stringPool));

  send_uart("Loading...\n");

  int result = loadFromFlash(program, programSize);
  if (result != 0) {
    send_uart("Load failed\n");
    while (1)
      ;
  }

  send_uart("Executing...\n");
  execute(g_bytecode, g_bytecodeSize, (const char**)g_stringPool, g_stringPoolSize, g_constPool);
  send_uart("Done\n");

  while (1)
    ;
}