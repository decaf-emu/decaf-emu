#pragma once
#include "types.h"

void *
memory_translate(ppcaddr_t address);

ppcaddr_t
memory_untranslate(const void *pointer);
