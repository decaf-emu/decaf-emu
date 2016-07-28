#include "common/decaf_assert.h"
#include "common/platform_dir.h"
#include "decaf_config.h"
#include "decaf_sound.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "snd_core.h"
#include "snd_core_voice.h"
#include <array>
#include <fstream>
#include <queue>

namespace snd_core
{

static const auto
MaxVoices = 96u;

static std::vector<AXVoice*>
sAcquiredVoices;

static std::queue<AXVoice*>
sAvailVoiceStack;

static std::array<internal::AXVoiceExtras, MaxVoices>
sVoiceExtras;

static void
createDumpDirectory()
{
   platform::createDirectory("dump");
}

static std::string
pointerAsString(const void *pointer)
{
   fmt::MemoryWriter format;
   format.write("{:08X}", mem::untranslate(pointer));
   return format.str();
}

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

   auto extras = internal::getVoiceExtras(foundVoice->index);
   for (auto i = 0; i < 6; ++i) {
      extras->tvVolume[i] = 0;
   }
   for (auto i = 0; i < 4; ++i) {
      extras->drcVolume[i] = 0;
      extras->controllerVolume[i] = 0;
   }
   extras->offsetFrac = 0;
   extras->loopCount = 0;
   extras->adpcmPrevSample[0] = 0;
   extras->adpcmPrevSample[1] = 0;

   // Save this to the acquired voice list so that it can be
   //  forcefully freed if a higher priority voice is needed.
   sAcquiredVoices.push_back(foundVoice);

   return foundVoice;
}

BOOL
AXCheckVoiceOffsets(AXVoiceOffsets *offsets)
{
   return TRUE;
}

void
AXFreeVoice(AXVoice *voice)
{
   auto voiceIter = std::find(sAcquiredVoices.begin(), sAcquiredVoices.end(), voice);
   decaf_check(voiceIter != sAcquiredVoices.end());

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

uint32_t
AXGetVoiceCurrentOffsetEx(AXVoice *voice,
                          const void *samples)
{
   AXVoiceOffsets offsets;
   AXGetVoiceOffsets(voice, &offsets);
   return offsets.currentOffset;
}

uint32_t
AXGetVoiceLoopCount(AXVoice *voice)
{
   return sVoiceExtras[voice->index].loopCount;
}

void
AXGetVoiceOffsets(AXVoice *voice,
                  AXVoiceOffsets *offsets)
{
   if (!decaf::getSoundDriver()) {
      // Trick the game into thinking the audio is progressing.
      voice->offsets.currentOffset += 20;

      if (voice->offsets.currentOffset > voice->offsets.endOffset) {
         if (voice->offsets.loopingEnabled) {
            voice->offsets.currentOffset -= (voice->offsets.endOffset - voice->offsets.loopOffset);
            internal::getVoiceExtras(voice->index)->loopCount++;
         } else {
            voice->offsets.currentOffset = voice->offsets.endOffset;
         }
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
AXSetVoiceAdpcm(AXVoice *voice,
                AXVoiceAdpcm *adpcm)
{
   auto extras = internal::getVoiceExtras(voice->index);
   for (auto i = 0; i < 16; ++i) {
      extras->adpcmCoeff[i] = adpcm->coefficients[i];
   }
   extras->adpcmPredScale = static_cast<uint8_t>(adpcm->predScale);
   extras->adpcmPrevSample[0] = adpcm->prevSample[0];
   extras->adpcmPrevSample[1] = adpcm->prevSample[1];

   if (decaf::config::sound::dump_sounds) {
      auto filename = "sound_" + pointerAsString(voice->offsets.data.get());

      if (!platform::fileExists("dump/" + filename + ".adpcm")) {
         createDumpDirectory();

         auto file = std::ofstream { "dump/" + filename + ".adpcm", std::ofstream::out };
         file.write(reinterpret_cast<char*>(adpcm), sizeof(*adpcm));
      }
   }
}

void
AXSetVoiceAdpcmLoop(AXVoice *voice,
                    AXVoiceAdpcmLoopData *loopData)
{
   auto extras = internal::getVoiceExtras(voice->index);
   extras->adpcmLoopPredScale = static_cast<uint8_t>(loopData->predScale);
   extras->adpcmLoopPrevSample[0] = loopData->prevSample[0];
   extras->adpcmLoopPrevSample[1] = loopData->prevSample[1];
}

void
AXSetVoiceCurrentOffset(AXVoice *voice,
                        uint32_t offset)
{
   voice->offsets.currentOffset = offset;
}

AXResult
AXSetVoiceDeviceMix(AXVoice *voice,
                    AXDeviceType type,
                    uint32_t id,
                    AXVoiceDeviceMixData *mixData)
{
   auto extras = internal::getVoiceExtras(voice->index);

   switch (type) {
   case AXDeviceType::TV:
      for (auto i = 0; i < 6; ++i) {
         extras->tvVolume[i] = mixData[i].volume;
      }
      break;
   case AXDeviceType::DRC:
      for (auto i = 0; i < 4; ++i) {
         extras->drcVolume[i] = mixData[i].volume;
      }
      break;
   case AXDeviceType::Controller:
      if (id < 4) {
         extras->controllerVolume[id] = mixData[id].volume;
      }
      break;
   }

   return AXResult::Success;
}

void
AXSetVoiceEndOffset(AXVoice *voice,
                    uint32_t offset)
{
   voice->offsets.endOffset = offset;
}

void
AXSetVoiceEndOffsetEx(AXVoice *voice,
                      uint32_t offset,
                      const void *samples)
{
   AXSetVoiceEndOffset(voice, offset);
}

void
AXSetVoiceLoopOffset(AXVoice *voice,
                     uint32_t offset)
{
   voice->offsets.loopOffset = offset;
}

void
AXSetVoiceLoopOffsetEx(AXVoice *voice,
                       uint32_t offset,
                       const void *samples)
{
   AXSetVoiceLoopOffset(voice, offset);
}

void
AXSetVoiceLoop(AXVoice *voice,
               AXVoiceLoop loop)
{
   voice->offsets.loopingEnabled = loop;
}

void
AXSetVoiceOffsets(AXVoice *voice,
                  AXVoiceOffsets *offsets)
{
   voice->offsets = *offsets;

   bool knownType = false;
   switch (offsets->dataType) {  // Let compiler warn us if we forget any types
   case AXVoiceFormat::ADPCM:
   case AXVoiceFormat::LPCM16:
   case AXVoiceFormat::LPCM8:
      knownType = true;
      break;
   }
   if (!knownType) {
      gLog->warn("AXSetVoiceOffsets: Unsupported data type {}", offsets->dataType.value());
   }

   if (decaf::config::sound::dump_sounds) {
      auto filename = "sound_" + pointerAsString(offsets->data.get());

      if (!platform::fileExists("dump/" + filename + ".txt")) {
         createDumpDirectory();

         auto file = std::ofstream { "dump/" + filename + ".txt", std::ofstream::out };
         auto format = fmt::MemoryWriter {};
         format
            << "offsets.dataType = " << static_cast<int>(offsets->dataType) << '\n'
            << "offsets.loopingEnabled = " << offsets->loopingEnabled << '\n'
            << "offsets.loopOffset = " << offsets->loopOffset << '\n'
            << "offsets.endOffset = " << offsets->endOffset << '\n'
            << "offsets.currentOffset = " << offsets->currentOffset << '\n'
            << "offsets.data = " << pointerAsString(offsets->data.get()) << '\n';
         file << format.str();
      }

      if (!platform::fileExists("dump/" + filename + ".bin")) {
         uint32_t bitsPerSample = 8;
         switch (offsets->dataType) {
         case AXVoiceFormat::ADPCM:
            bitsPerSample = 4;
            break;
         case AXVoiceFormat::LPCM8:
            bitsPerSample = 8;
            break;
         case AXVoiceFormat::LPCM16:
            bitsPerSample = 16;
            break;
         }
         uint32_t streamBytes = (offsets->endOffset + 1) * bitsPerSample / 8;
         auto file = std::ofstream { "dump/" + filename + ".bin", std::ofstream::out };
         file.write(reinterpret_cast<char*>(offsets->data.get()), streamBytes);
      }
   }
}

void
AXSetVoicePriority(AXVoice *voice,
                   uint32_t priority)
{
   voice->priority = priority;
}

void
AXSetVoiceRmtIIRCoefs(AXVoice *voice,
                      uint16_t filter,
                      ppctypes::VarList &args)
{
}

void
AXSetVoiceSrc(AXVoice *voice,
              AXVoiceSrc *src)
{
   auto extras = internal::getVoiceExtras(voice->index);
   extras->playbackRatio = static_cast<uint32_t>(src->ratioInt) << 16 | src->ratioFrac;
}

AXVoiceSrcRatioResult
AXSetVoiceSrcRatio(AXVoice *voice,
                   float ratio)
{
   if (ratio < 0.0f) {
      return AXVoiceSrcRatioResult::RatioLessThanZero;
   }

   auto extras = internal::getVoiceExtras(voice->index);
   extras->playbackRatio = static_cast<uint32_t>(ratio * (1 << 16));
   return AXVoiceSrcRatioResult::Success;
}

void
AXSetVoiceSrcType(AXVoice *voice,
                  AXVoiceSrcType type)
{
}

void
AXSetVoiceState(AXVoice *voice,
                AXVoiceState state)
{
   voice->state = state;
}

void
AXSetVoiceType(AXVoice *voice,
               AXVoiceType type)
{
}

void
AXSetVoiceVe(AXVoice *voice,
             AXVoiceVeData *veData)
{
}

namespace internal
{

void
initVoices()
{
   for (auto i = 0; i < MaxVoices; ++i) {
      auto newVoice = coreinit::internal::sysAlloc<AXVoice>();
      newVoice->index = i;
      sAvailVoiceStack.push(newVoice);
   }
}

const std::vector<AXVoice*>
getAcquiredVoices()
{
   return sAcquiredVoices;
}

AXVoiceExtras *
getVoiceExtras(int index)
{
   decaf_check(index >= 0 && index < MaxVoices);
   return &sVoiceExtras[index];
}

} // namespace internal

void
Module::registerVoiceFunctions()
{
   RegisterKernelFunction(AXAcquireVoice);
   RegisterKernelFunction(AXAcquireVoiceEx);
   RegisterKernelFunction(AXCheckVoiceOffsets);
   RegisterKernelFunction(AXFreeVoice);
   RegisterKernelFunction(AXGetMaxVoices);
   RegisterKernelFunction(AXGetVoiceCurrentOffsetEx);
   RegisterKernelFunction(AXGetVoiceLoopCount);
   RegisterKernelFunction(AXGetVoiceOffsets);
   RegisterKernelFunction(AXIsVoiceRunning);
   RegisterKernelFunction(AXSetVoiceAdpcm);
   RegisterKernelFunction(AXSetVoiceAdpcmLoop);
   RegisterKernelFunction(AXSetVoiceCurrentOffset);
   RegisterKernelFunction(AXSetVoiceDeviceMix);
   RegisterKernelFunction(AXSetVoiceEndOffset);
   RegisterKernelFunction(AXSetVoiceEndOffsetEx);
   RegisterKernelFunction(AXSetVoiceLoopOffset);
   RegisterKernelFunction(AXSetVoiceLoopOffsetEx);
   RegisterKernelFunction(AXSetVoiceLoop);
   RegisterKernelFunction(AXSetVoiceOffsets);
   RegisterKernelFunction(AXSetVoicePriority);
   RegisterKernelFunction(AXSetVoiceRmtIIRCoefs);
   RegisterKernelFunction(AXSetVoiceSrc);
   RegisterKernelFunction(AXSetVoiceSrcType);
   RegisterKernelFunction(AXSetVoiceSrcRatio);
   RegisterKernelFunction(AXSetVoiceState);
   RegisterKernelFunction(AXSetVoiceType);
   RegisterKernelFunction(AXSetVoiceVe);
}

} // namespace snd_core
