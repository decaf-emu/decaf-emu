#pragma once
#include "kernelmodule.h"
#include "log.h"

class Zlib125 : public KernelModuleImpl<Zlib125>
{
public:
   Zlib125();
   virtual ~Zlib125() override;

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};
