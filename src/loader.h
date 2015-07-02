#pragma once
#include <string>
#include <vector>

struct UserModule;

class Loader
{
public:
   bool loadRPL(UserModule &module, const char *buffer, size_t size);

private:
};
