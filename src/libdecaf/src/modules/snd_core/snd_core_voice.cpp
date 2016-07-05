#include "common/emuassert.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "snd_core.h"
#include "snd_core_voice.h"
#include <queue>

namespace snd_core
{

static const int MaxVoices = 6;

static std::vector<AXVoice*>
sAcquiredVoices;

static std::queue<AXVoice*>
sAvailVoiceStack;

AXVoice *
AXAcquireVoice(uint32_t priority,
               AXVoiceCallbackFn callback,
               void *userContext)
{
   // Because the callback first parameter matches, we can simply forward
   //  it to AXAcquireVoiceEx and get the same effect. COS does this too.
   return AXAcquireVoiceEx(priority, callback.getAddress(), userContext);
}

AXVoice *
AXAcquireVoiceEx(uint32_t priority,
                 AXVoiceCallbackExFn callback,
                 void *userContext)
{
   if (sAvailVoiceStack.empty()) {
      // If there are no available voices, try to force-deallocate one...
      for (auto &i : sAcquiredVoices) {
         if (i->priority < priority) {
            // TODO: Send callback for forcing the FreeVoice
            AXFreeVoice(i);
            break;
         }
      }
   }

   if (sAvailVoiceStack.empty()) {
      // No voices available to acquire
      return nullptr;
   }

   // Grab our voice from the stack
   AXVoice *foundVoice = sAvailVoiceStack.front();
   sAvailVoiceStack.pop();

   // Configure the voice with stuff we know about
   foundVoice->priority = priority;
   foundVoice->callbackEx = callback;
   foundVoice->userContext = userContext;

   // Save this to the acquired voice list so that it can be
   //  forcefully freed if a higher priority voice is needed.
   sAcquiredVoices.push_back(foundVoice);

   return foundVoice;
}

void
AXFreeVoice(AXVoice *voice)
{
   auto voiceIter = std::find(sAcquiredVoices.begin(), sAcquiredVoices.end(), voice);
   emuassert(voiceIter != sAcquiredVoices.end());

   // Erase this voice from the acquired list
   sAcquiredVoices.erase(voiceIter);

   // Make this voice available on the available stack!
   sAvailVoiceStack.push(voice);
}

uint32_t
AXGetMaxVoices()
{
   return MaxVoices;
}

void
AXSetVoiceVe(AXVoice *voice,
             AXVoiceVeData *veData)
{
}

void
AXSetVoiceOffsets(AXVoice *voice,
                  AXVoiceOffsets *offsets)
{
   voice->offsets = *offsets;
}

void
AXGetVoiceOffsets(AXVoice *voice,
                  AXVoiceOffsets *offsets)
{
   // Trick the game into thinking the audio is progressing.
   voice->offsets.currentOffset += 20;

   if (voice->offsets.currentOffset > voice->offsets.endOffset) {
      if (voice->offsets.loopingEnabled) {
         voice->offsets.currentOffset -= (voice->offsets.endOffset - voice->offsets.loopOffset);
      } else {
         voice->offsets.currentOffset = voice->offsets.endOffset;
      }
   }

   *offsets = voice->offsets;
}

BOOL
AXIsVoiceRunning(AXVoice *voice)
{
   return FALSE;
}

void
AXSetVoiceAdpcmLoop(AXVoice *voice,
                    AXVoiceAdpcmLoopData *loopData)
{
}

AXResult
AXSetVoiceDeviceMix(AXVoice *voice,
                    AXDeviceType type,
                    uint32_t id,
                    AXVoiceDeviceMixData *mixData)
{
   return AXResult::Success;
}

void
AXSetVoiceLoop(AXVoice *voice,
               AXVoiceLoop loop)
{
   voice->offsets.loopingEnabled = loop;
}

void
AXSetVoiceState(AXVoice *voice,
                AXVoiceState state)
{
   voice->state = state;
}

AXVoiceSrcRatioResult
AXSetVoiceSrcRatio(AXVoice *voice,
                   float ratio)
{
   return AXVoiceSrcRatioResult::Success;
}

void
AXSetVoiceEndOffset(AXVoice *voice,
                    uint32_t offset)
{
   voice->offsets.endOffset = offset;
}

namespace internal
{

void initVoices()
{
   for (auto i = 0; i < MaxVoices; ++i) {
      auto newVoice = coreinit::internal::sysAlloc<AXVoice>();
      newVoice->index = i;
      sAvailVoiceStack.push(newVoice);
   }
}

} // namespace internal

void
Module::registerVoiceFunctions()
{
   RegisterKernelFunction(AXAcquireVoice);
   RegisterKernelFunction(AXAcquireVoiceEx);
   RegisterKernelFunction(AXFreeVoice);
   RegisterKernelFunction(AXGetMaxVoices);
   RegisterKernelFunction(AXSetVoiceVe);
   RegisterKernelFunction(AXIsVoiceRunning);
   RegisterKernelFunction(AXSetVoiceOffsets);
   RegisterKernelFunction(AXGetVoiceOffsets);
   RegisterKernelFunction(AXSetVoiceAdpcmLoop);
   RegisterKernelFunction(AXSetVoiceDeviceMix);
   RegisterKernelFunction(AXSetVoiceState);
   RegisterKernelFunction(AXSetVoiceLoop);
   RegisterKernelFunction(AXSetVoiceSrcRatio);
   RegisterKernelFunction(AXSetVoiceEndOffset);
}

} // namespace snd_core
