#pragma once
#include "systemmodule.h"
#include "log.h"

class GX2 : public SystemModuleImpl<GX2>
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
   static void registerResourceFunctions();
   static void registerRenderStateFunctions();
   static void registerShaderFunctions();
   static void registerSurfaceFunctions();
   static void registerTempFunctions();
   static void registerTextureFunctions();
};
