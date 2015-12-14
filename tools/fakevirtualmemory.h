#pragma once
#include <cstdint>

void
memory_virtualmap(uint32_t addr, void *pointer);

uint32_t
memory_virtualmap(void *pointer);
