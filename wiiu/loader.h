#pragma once
#include <string>
#include <vector>
#include "module.h"

struct EntryInfo
{
   uint32_t address;
   uint32_t stackSize;
};

class Loader
{
public:
   bool loadRPL(Module &module, EntryInfo &entry, const char *buffer, size_t size);

private:
};
