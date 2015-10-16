#include "mem/mem.h"
#include "memory_translate.h"

void *
memory_translate(ppcaddr_t address)
{
   return mem::translate<void>(address);
}

ppcaddr_t
memory_untranslate(const void *pointer)
{
   return mem::untranslate(pointer);
}
