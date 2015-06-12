#pragma once
#include "systemmodule.h"
#include "log.h"

class Zlib125 : public SystemModuleImpl<Zlib125>
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
