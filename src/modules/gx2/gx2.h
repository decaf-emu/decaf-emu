#pragma once
#include "kernelmodule.h"

static const int GX2_NUM_GPRS = 144;
static const int GX2_NUM_SAMPLERS = 4;
static const int GX2_NUM_UNIFORMBLOCKS = 16;
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
};
