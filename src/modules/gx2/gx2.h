#pragma once
#include "kernelmodule.h"

class GX2 : public KernelModuleImpl<GX2>
{
public:
   GX2();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
   static void registerDisplayFunctions();
   static void registerDisplayListFunctions();
   static void registerDrawFunctions();
   static void registerResourceFunctions();
   static void registerRenderStateFunctions();
   static void registerShaderFunctions();
   static void registerSurfaceFunctions();
   static void registerTempFunctions();
   static void registerTextureFunctions();
};
