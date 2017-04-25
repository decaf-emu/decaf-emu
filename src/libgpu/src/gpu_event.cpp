#include "gpu.h"
#include "gpu_event.h"

namespace gpu
{

static FlipCallbackFn
sFlipCallbackFn = nullptr;

static RetireCallbackFn
sRetireCallbackFn = nullptr;

static SyncRegisterCallbackFn
sSyncRegisterCallbackFn = nullptr;

void
setFlipCallback(FlipCallbackFn callback)
{
   sFlipCallbackFn = callback;
}

void
setRetireCallback(RetireCallbackFn callback)
{
   sRetireCallbackFn = callback;
}

void
setSyncRegisterCallback(SyncRegisterCallbackFn callback)
{
   sSyncRegisterCallbackFn = callback;
}

void
onFlip()
{
   if (sFlipCallbackFn) {
      sFlipCallbackFn();
   }
}

void
onRetire(void *context)
{
   if (sRetireCallbackFn) {
      sRetireCallbackFn(context);
   }
}

void
onSyncRegisters(const uint32_t *registers,
                uint32_t size)
{
   if (sSyncRegisterCallbackFn) {
      sSyncRegisterCallbackFn(registers, size);
   }
}

} // namespace gpu
