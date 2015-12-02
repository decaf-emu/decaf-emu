#pragma once
#include "kernelmodule.h"

class Zlib125 : public KernelModuleImpl<Zlib125>
{
public:
   virtual ~Zlib125() = default;

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};
