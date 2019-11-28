#include "gpu.h"
#include "gpu_event.h"

namespace gpu
{

static FlipCallbackFn sFlipCallbackFn = nullptr;

void
setFlipCallback(FlipCallbackFn callback)
{
   sFlipCallbackFn = callback;
}

void
onFlip()
{
   if (sFlipCallbackFn) {
      sFlipCallbackFn();
   }
}

} // namespace gpu
