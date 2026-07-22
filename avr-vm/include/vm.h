#pragma once

#include "includes.h"

int execute(
  const uint8_t* bytecode,
  const int bytecodeSize,
  const char* stringPool[],
  const int stringPoolSize,
  const float* g_constPool);