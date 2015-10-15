#include "memory.h"
#include "memory_translate.h"

void *
memory_translate(ppcaddr_t address)
{
   return gMemory.translate<void>(address);
}

ppcaddr_t
memory_untranslate(const void *pointer)
{
   return gMemory.untranslate(pointer);
}
