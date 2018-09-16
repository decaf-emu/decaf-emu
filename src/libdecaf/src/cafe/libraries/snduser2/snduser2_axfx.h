#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::snduser2
{
using AXFXAllocFuncPtr = virt_func_ptr<virt_ptr<void>(uint32_t size)>;
using AXFXFreeFuncPtr = virt_func_ptr<void(virt_ptr<void> ptr)>;

struct AXAuxCallbackData
{
   be2_val<uint32_t> channels;
   be2_val<uint32_t> samples;
};
CHECK_OFFSET(AXAuxCallbackData, 0x0, channels);
CHECK_OFFSET(AXAuxCallbackData, 0x4, samples);
CHECK_SIZE(AXAuxCallbackData, 0x8);

struct AXFXBuffers
{
   virt_ptr<int32_t> left;
   virt_ptr<int32_t> right;
   virt_ptr<int32_t> surround;
};
CHECK_OFFSET(AXFXBuffers, 0x00, left);
CHECK_OFFSET(AXFXBuffers, 0x04, right);
CHECK_OFFSET(AXFXBuffers, 0x08, surround);
CHECK_SIZE(AXFXBuffers, 0x0C);

#pragma pack(push, 1)
struct AXFXReverbHi;
struct AXFXDelay;
struct AXFXChorus;
struct AXFXReverbStd;
struct AXFXReverbMulti;

struct StaticFxData
{
   be2_val<AXFXAllocFuncPtr> allocFuncPtr;
   be2_val<AXFXFreeFuncPtr> freeFuncPtr;
};

void
AXFXSetHooks(AXFXAllocFuncPtr allocFn,
             AXFXFreeFuncPtr freeFn);
void
AXFXGetHooks(virt_ptr<AXFXAllocFuncPtr> allocFn,
             virt_ptr<AXFXFreeFuncPtr> freeFn);

int32_t
AXFXChorusGetMemSize(virt_ptr<AXFXChorus> chorus);

int32_t
AXFXChorusExpGetMemSize(virt_ptr<AXFXChorus> chorus);

int32_t
AXFXDelayGetMemSize(virt_ptr<AXFXDelay> delay);

int32_t
AXFXDelayExpGetMemSize(virt_ptr<AXFXDelay> delay);

int32_t
AXFXReverbHiGetMemSize(virt_ptr<AXFXReverbHi> reverb);

int32_t
AXFXReverbHiExpGetMemSize(virt_ptr<AXFXReverbHi> reverb);

int32_t
AXFXReverbStdGetMemSize(virt_ptr<AXFXReverbStd> reverb);

int32_t
AXFXReverbStdExpGetMemSize(virt_ptr<AXFXReverbStd> reverb);

void
AXFXChorusCallback(virt_ptr<AXFXBuffers> buffers,
                   virt_ptr<AXFXChorus> data,
                   virt_ptr<AXAuxCallbackData> auxData);

void
AXFXChorusExpCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXChorus> data);

void
AXFXDelayCallback(virt_ptr<AXFXBuffers> buffers,
                  virt_ptr<AXFXDelay> data,
                  virt_ptr<AXAuxCallbackData> auxData);

void
AXFXDelayExpCallback(virt_ptr<AXFXBuffers> buffers,
                     virt_ptr<AXFXDelay> data);

void
AXFXReverbHiCallback(virt_ptr<AXFXBuffers> buffers,
                     virt_ptr<AXFXReverbHi> data,
                     virt_ptr<AXAuxCallbackData> auxData);

void
AXFXReverbHiExpCallback(virt_ptr<AXFXBuffers> buffers,
                        virt_ptr<AXFXReverbHi> data);

void
AXFXMultiChReverbCallback(virt_ptr<AXFXBuffers> buffers,
                          virt_ptr<AXFXReverbMulti> data,
                          virt_ptr<AXAuxCallbackData> auxData);

void
AXFXReverbStdCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXReverbStd> data,
                      virt_ptr<AXAuxCallbackData> auxData);

void
AXFXReverbStdExpCallback(virt_ptr<AXFXBuffers> buffers,
                         virt_ptr<AXFXReverbStd> data);

BOOL
AXFXChorusInit(virt_ptr<AXFXChorus> chorus);

BOOL
AXFXChorusExpInit(virt_ptr<AXFXChorus> chorus);

BOOL
AXFXDelayInit(virt_ptr<AXFXDelay> delay);

BOOL
AXFXDelayExpInit(virt_ptr<AXFXDelay> delay);

BOOL
AXFXReverbHiExpInit(virt_ptr<AXFXReverbHi> reverb);

BOOL
AXFXReverbStdInit(virt_ptr<AXFXReverbHi> reverb);

BOOL
AXFXReverbStdExpInit(virt_ptr<AXFXReverbHi> reverb);

void
AXFXChorusShutdown(virt_ptr<AXFXChorus> chorus);

void
AXFXChorusExpShutdown(virt_ptr<AXFXChorus> chorus);

void
AXFXDelayShutdown(virt_ptr<AXFXDelay> delay);

void
AXFXDelayExpShutdown(virt_ptr<AXFXDelay> delay);

void
AXFXReverbHiExpShutdown(virt_ptr<AXFXReverbHi> reverb);

void
AXFXReverbStdShutdown(virt_ptr<AXFXReverbHi> reverb);

void
AXFXReverbStdExpShutdown(virt_ptr<AXFXReverbHi> reverb);

BOOL
AXFXReverbHiInit(virt_ptr<AXFXReverbHi> reverb);

BOOL
AXFXReverbHiShutdown(virt_ptr<AXFXReverbHi> reverb);

BOOL
AXFXReverbHiSettings();

BOOL
AXFXMultiChReverbInit();

} // namespace cafe::snduser2
