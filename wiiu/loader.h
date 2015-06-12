#pragma once
#include <string>
#include <vector>

struct UserModule;

struct EntryInfo
{
   uint32_t address;
   uint32_t stackSize;
};

class Loader
{
public:
   bool loadRPL(UserModule &module, EntryInfo &entry, const char *buffer, size_t size);

private:
};
