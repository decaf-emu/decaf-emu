#pragma once
#include "kernelmodule.h"

namespace zlib125
{

class Module : public KernelModuleImpl<Module>
{
public:
   virtual ~Module() = default;

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};

} // namespace zlib125
