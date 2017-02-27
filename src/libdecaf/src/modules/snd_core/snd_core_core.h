#pragma once
#include "ppcutils/wfunc_ptr.h"
#include "snd_core_enum.h"

#include <common/be_val.h>
#include <common/cbool.h>
#include <cstdint>

namespace snd_core
{

#pragma pack(push, 1)

struct AXProfile;

struct AXInitParams
{
   be_val<AXInitRenderer> renderer;
   UNKNOWN(4);
   be_val<AXInitPipeline> pipeline;
};

#pragma pack(pop)

using AXFrameCallback = wfunc_ptr<void>;

void
AXInit();

void
AXInitWithParams(AXInitParams *params);

BOOL
AXIsInit();

void
AXInitProfile(AXProfile *profile,
              uint32_t count);

uint32_t
AXGetSwapProfile(AXProfile *profile,
                 uint32_t count);

AXResult
AXSetDefaultMixerSelect(uint32_t);

AXFrameCallback
AXRegisterFrameCallback(AXFrameCallback callback);

AXResult
AXRegisterAppFrameCallback(AXFrameCallback callback);

AXResult
AXDeregisterAppFrameCallback(AXFrameCallback callback);

uint32_t
AXGetInputSamplesPerFrame();

uint32_t
AXGetInputSamplesPerSec();

int32_t
AXRmtGetSamplesLeft();

int32_t
AXRmtGetSamples(int32_t,
                be_val<uint8_t> *buffer,
                int32_t samples);

int32_t
AXRmtAdvancePtr(int32_t);

void
AXPrepareEfxData(void *buffer, uint32_t size);

namespace internal
{

void
initEvents();

int
getOutputRate();

} // namespace internal

} // namespace snd_core
