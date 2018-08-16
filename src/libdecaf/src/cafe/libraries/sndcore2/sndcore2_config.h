#pragma once
#include "sndcore2_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::sndcore2
{

#pragma pack(push, 1)

struct AXProfile;

struct AXInitParams
{
   be2_val<AXRendererFreq> renderer;
   UNKNOWN(4);
   be2_val<AXInitPipeline> pipeline;
};
CHECK_OFFSET(AXInitParams, 0x00, renderer);
CHECK_OFFSET(AXInitParams, 0x08, pipeline);
CHECK_SIZE(AXInitParams, 0x0C);

#pragma pack(pop)

using AXFrameCallback = virt_func_ptr<
   void()
>;

void
AXInit();

void
AXInitWithParams(virt_ptr<AXInitParams> params);

BOOL
AXIsInit();

void
AXQuit();

void
AXInitProfile(virt_ptr<AXProfile> profile,
              uint32_t count);

AXRendererFreq
AXGetRendererFreq();

uint32_t
AXGetSwapProfile(virt_ptr<AXProfile> profile,
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

void
AXPrepareEfxData(virt_ptr<void> buffer,
                 uint32_t size);

int32_t
AXUserBegin();

int32_t
AXUserEnd();

BOOL
AXUserIsProtected();

namespace internal
{

void
initEvents();

int
getOutputRate();

} // namespace internal

} // namespace cafe::sndcore2
