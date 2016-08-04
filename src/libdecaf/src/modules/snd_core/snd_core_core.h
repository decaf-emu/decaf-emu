#pragma once
#include "ppcutils/wfunc_ptr.h"
#include "snd_core_enum.h"

namespace snd_core
{

// TODO: Reverse AXProfile
struct AXProfile;

using AXFrameCallback = wfunc_ptr<void>;

void
AXInit();

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

AXResult
AXRegisterAppFrameCallback(AXFrameCallback callback);

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

namespace internal
{

void
initEvents();

int
getOutputRate();

} // namespace internal

} // namespace snd_core
