#include "common/decaf_assert.h"
#include "common/platform_dir.h"
#include "decaf_config.h"
#include "decaf_sound.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "snd_core.h"
#include "snd_core_core.h"
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
   std::memset(extras, 0, sizeof(internal::AXVoiceExtras));
   extras->src.ratio = 1.0;

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
   *offsets = voice->offsets;
}

BOOL
AXIsVoiceRunning(AXVoice *voice)
{
   auto extras = internal::getVoiceExtras(voice->index);
   return (extras->state != AXVoiceState::Stopped) ? 1 : 0;
}

void
AXSetVoiceAdpcm(AXVoice *voice,
                AXVoiceAdpcm *adpcm)
{
   auto extras = internal::getVoiceExtras(voice->index);
   extras->adpcm = *adpcm;
   extras->syncBits |= internal::AXVoiceSyncBits::Adpcm;
}

void
AXSetVoiceAdpcmLoop(AXVoice *voice,
                    AXVoiceAdpcmLoopData *loopData)
{
   auto extras = internal::getVoiceExtras(voice->index);
   extras->adpcmLoop = *loopData;
   extras->syncBits |= internal::AXVoiceSyncBits::AdpcmLoop;
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
                    uint32_t deviceId,
                    AXVoiceDeviceMixData *mixData)
{
   auto extras = internal::getVoiceExtras(voice->index);

   switch (type) {
   case AXDeviceType::TV:
      decaf_check(deviceId < AXNumTvDevices);

      for (auto c = 0; c < AXNumTvChannels; ++c) {
         for (auto b = 0; b < AXNumTvBus; ++b) {
            extras->tvVolume[deviceId][c][b].volume = ufixed_1_15_t::from_data(mixData[c].bus[b].volume);
            extras->tvVolume[deviceId][c][b].delta = ufixed_1_15_t::from_data(mixData[c].bus[b].delta);
         }
      }

      break;
   case AXDeviceType::DRC:
      decaf_check(deviceId < AXNumDrcDevices);

      for (auto c = 0; c < AXNumDrcChannels; ++c) {
         for (auto b = 0; b < AXNumDrcBus; ++b) {
            extras->drcVolume[deviceId][c][b].volume = ufixed_1_15_t::from_data(mixData[c].bus[b].volume);
            extras->drcVolume[deviceId][c][b].delta = ufixed_1_15_t::from_data(mixData[c].bus[b].delta);
         }
      }

      break;
   case AXDeviceType::RMT:
      decaf_check(deviceId < AXNumRmtDevices);

      for (auto c = 0; c < AXNumRmtChannels; ++c) {
         for (auto b = 0; b < AXNumRmtBus; ++b) {
            extras->rmtVolume[deviceId][c][b].volume = ufixed_1_15_t::from_data(mixData[c].bus[b].volume);
            extras->rmtVolume[deviceId][c][b].delta = ufixed_1_15_t::from_data(mixData[c].bus[b].delta);
         }
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
   voice->offsets.data = samples;
}

AXResult
AXSetVoiceInitialTimeDelay(AXVoice *voice,
                           uint16_t delay)
{
   if (AXIsVoiceRunning(voice)) {
      return AXResult::VoiceIsRunning;
   }

   if (delay > AXGetInputSamplesPerFrame()) {
      return AXResult::DelayTooBig;
   }

   auto extras = internal::getVoiceExtras(voice->index);
   extras->itdOn = 1;
   extras->itdDelay = delay;
   extras->syncBits |= internal::AXVoiceSyncBits::Itd;
   voice->syncBits |= internal::AXVoiceSyncBits::Itd;

   return AXResult::Success;
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
   voice->offsets.data = samples;
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
   decaf_check(offsets->dataType == AXVoiceFormat::ADPCM ||
      offsets->dataType == AXVoiceFormat::LPCM16 ||
      offsets->dataType == AXVoiceFormat::LPCM8);

   voice->offsets = *offsets;

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
         file.write(reinterpret_cast<const char*>(offsets->data.get()), streamBytes);
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
                      ppctypes::VarArgs)
{
}

void
AXSetVoiceSrc(AXVoice *voice,
              AXVoiceSrc *src)
{
   auto extras = internal::getVoiceExtras(voice->index);
   extras->src = *src;
   voice->syncBits |= internal::AXVoiceSyncBits::Src;
}

AXVoiceSrcRatioResult
AXSetVoiceSrcRatio(AXVoice *voice,
                   float ratio)
{
   if (ratio < 0.0f) {
      return AXVoiceSrcRatioResult::RatioLessThanZero;
   }

   auto extras = internal::getVoiceExtras(voice->index);
   extras->src.ratio = ratio;
   voice->syncBits |= internal::AXVoiceSyncBits::SrcRatio;

   return AXVoiceSrcRatioResult::Success;
}

void
AXSetVoiceSrcType(AXVoice *voice,
                  AXVoiceSrcType type)
{
   auto extras = internal::getVoiceExtras(voice->index);

   if (type == AXVoiceSrcType::None) {
      extras->srcMode = 2;
   } else if (type == AXVoiceSrcType::Linear) {
      extras->srcMode = 1;
   } else if (type == AXVoiceSrcType::Unk0) {
      extras->srcMode = 0;
      extras->srcModeUnk = 0;
   } else if (type == AXVoiceSrcType::Unk1) {
      extras->srcMode = 0;
      extras->srcModeUnk = 1;
   } else if (type == AXVoiceSrcType::Unk2) {
      extras->srcMode = 0;
      extras->srcModeUnk = 2;
   }

   voice->syncBits |= internal::AXVoiceSyncBits::SrcType;
}

void
AXSetVoiceState(AXVoice *voice,
                AXVoiceState state)
{
   auto extras = internal::getVoiceExtras(voice->index);
   extras->state = state;
   voice->state = state;
   voice->syncBits |= internal::AXVoiceSyncBits::State;
}

void
AXSetVoiceType(AXVoice *voice,
               AXVoiceType type)
{
   auto extras = internal::getVoiceExtras(voice->index);
   extras->type = type;
   voice->syncBits |= internal::AXVoiceSyncBits::Type;
}

void
AXSetVoiceVe(AXVoice *voice,
             AXVoiceVeData *veData)
{
   auto extras = internal::getVoiceExtras(voice->index);
   extras->ve = *veData;
   voice->syncBits |= internal::AXVoiceSyncBits::Ve;
}

void
AXSetVoiceVeDelta(AXVoice *voice,
                  int16_t delta)
{
   auto extras = internal::getVoiceExtras(voice->index);
   if (extras->ve.delta != delta) {
      extras->ve.delta = delta;
      voice->syncBits |= internal::AXVoiceSyncBits::VeDelta;
   }
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
   RegisterKernelFunction(AXSetVoiceInitialTimeDelay);
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
   RegisterKernelFunction(AXSetVoiceVeDelta);
}

} // namespace snd_core
