#pragma once
#include "kernelmodule.h"

//#define GX2_DX11
#define GX2_DX12
//#define GX2_GL40
//#define GX2_GL50

#define GX2_DUMP_SHADERS

#if !( defined(GX2_DX11) || defined(GX2_DX12) || defined(GX2_GL40) || defined(GX2_GL50) )
#define GX2_NULL
#endif

static const int GX2_NUM_GPRS = 144;
static const int GX2_NUM_SAMPLERS = 8;
static const int GX2_NUM_UNIFORMBLOCKS = 14; // Supposed to be 16, but DX12 doesn't actually support that...
static const int GX2_NUM_MRT_BUFFER = 8;
static const int GX2_NUM_TEXTURE_UNIT = 16;

class GX2 : public KernelModuleImpl<GX2>
{
public:
   GX2();

   virtual void initialise() override;

   void initialiseVsync();

public:
   static void RegisterFunctions();

private:
   static void registerContextFunctions();
   static void registerDisplayFunctions();
   static void registerDisplayListFunctions();
   static void registerDrawFunctions();
   static void registerResourceFunctions();
   static void registerRenderStateFunctions();
   static void registerShaderFunctions();
   static void registerSurfaceFunctions();
   static void registerTempFunctions();
   static void registerTextureFunctions();
   static void registerVsyncFunctions();
};
