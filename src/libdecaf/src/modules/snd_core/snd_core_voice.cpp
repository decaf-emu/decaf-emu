#include "decaf_config.h"
#include "decaf_sound.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "snd_core.h"
#include "snd_core_core.h"
#include "snd_core_voice.h"

#include <common/decaf_assert.h>
#include <common/platform_dir.h>
#include <array>
#include <fmt/format.h>
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

   // Reset the voice
   auto voiceIndex = foundVoice->index;
   memset(foundVoice, 0, sizeof(AXVoice));
   foundVoice->index = voiceIndex;

   // Configure the voice with stuff we know about
   foundVoice->priority = priority;
   foundVoice->callbackEx = callback;
   foundVoice->userContext = userContext;

   auto extras = internal::getVoiceExtras(foundVoice->index);
   std::memset(extras, 0, sizeof(internal::AXVoiceExtras));
   extras->src.ratio = ufixed1616_t { 1.0 };

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
   AXGetVoiceOffsetsEx(voice, &offsets, samples);
   return offsets.currentOffset;
}

uint32_t
AXGetVoiceLoopCount(AXVoice *voice)
{
   return sVoiceExtras[voice->index].loopCount;
}

uint32_t
AXGetVoiceMixerSelect(AXVoice *voice)
{
   return voice->renderer;
}

void
AXGetVoiceOffsetsEx(AXVoice *voice,
                    AXVoiceOffsets *offsets,
                    const void *samples)
{
   voice->offsets.data = samples;
   AXGetVoiceOffsets(voice, offsets);
   voice->offsets = *offsets;
}

void
AXGetVoiceOffsets(AXVoice *voice,
                  AXVoiceOffsets *offsets)
{
   auto extras = internal::getVoiceExtras(voice->index);

   *offsets = voice->offsets;

   auto pageAddress = extras->data.memPageNumber << 29;
   auto baseAddress = offsets->data.getAddress();
   if (extras->data.format == AXVoiceFormat::ADPCM) {
      // We cheat an use our 64-bitness to avoid more complicated math requirements here
      auto pageAddressSamples = static_cast<uint64_t>(pageAddress) << 1;
      auto baseAddressSamples = static_cast<uint64_t>(baseAddress) << 1;
      offsets->loopOffset = static_cast<uint32_t>(pageAddressSamples + static_cast<uint64_t>(extras->data.loopOffsetAbs) - baseAddressSamples);
      offsets->endOffset = static_cast<uint32_t>(pageAddressSamples + static_cast<uint64_t>(extras->data.endOffsetAbs) - baseAddressSamples);
      offsets->currentOffset = static_cast<uint32_t>(pageAddressSamples + static_cast<uint64_t>(extras->data.currentOffsetAbs) - baseAddressSamples);
   } else if (extras->data.format == AXVoiceFormat::LPCM16) {
      auto pageAddressSamples = pageAddress >> 1;
      auto baseAddressSamples = baseAddress >> 1;
      offsets->loopOffset = pageAddressSamples + extras->data.loopOffsetAbs - baseAddressSamples;
      offsets->endOffset = pageAddressSamples + extras->data.endOffsetAbs - baseAddressSamples;
      offsets->currentOffset = pageAddressSamples + extras->data.currentOffsetAbs - baseAddressSamples;
   } else if (extras->data.format == AXVoiceFormat::LPCM8) {
      offsets->loopOffset = pageAddress + extras->data.loopOffsetAbs - baseAddress;
      offsets->endOffset = pageAddress + extras->data.endOffsetAbs - baseAddress;
      offsets->currentOffset = pageAddress + extras->data.currentOffsetAbs - baseAddress;
   } else {
      be_val<uint16_t> dataFormat = (uint16_t)extras->data.format;
      decaf_abort(fmt::format("Unexpected voice data format {}", dataFormat));
      //decaf_abort(fmt::format("Unexpected voice data format {}", extras->data.format))
   }
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
   auto extras = internal::getVoiceExtras(voice->index);

   auto baseAddress = voice->offsets.data.getAddress() & 0x1FFFFFFF;

   if (extras->data.format == AXVoiceFormat::ADPCM) {
      extras->data.currentOffsetAbs = (baseAddress << 1) + offset;
   } else if (extras->data.format == AXVoiceFormat::LPCM16) {
      extras->data.currentOffsetAbs = (baseAddress >> 1) + offset;
   } else if (extras->data.format == AXVoiceFormat::LPCM8) {
      extras->data.currentOffsetAbs = baseAddress + offset;
   } else {
      be_val<uint16_t> dataFormat = (uint16_t)extras->data.format;
      decaf_abort(fmt::format("Unexpected voice data format {}", dataFormat));
      //decaf_abort(fmt::format("Unexpected voice data type {}", extras->data.format));
   }

   extras->syncBits |= internal::AXVoiceSyncBits::CurrentOffset;
}

void
AXSetVoiceCurrentOffsetEx(AXVoice *voice,
                          uint32_t offset,
                          const void *samples)
{
   voice->offsets.data = samples;

   AXVoiceOffsets offsets;
   AXGetVoiceOffsets(voice, &offsets);

   AXSetVoiceCurrentOffset(voice, offset);
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
   auto extras = internal::getVoiceExtras(voice->index);

   voice->offsets.endOffset = offset;

   auto baseAddress = voice->offsets.data.getAddress() & 0x1FFFFFFF;

   if (extras->data.format == AXVoiceFormat::ADPCM) {
      extras->data.endOffsetAbs = (baseAddress << 1) + offset;
   } else if (extras->data.format == AXVoiceFormat::LPCM16) {
      extras->data.endOffsetAbs = (baseAddress >> 1) + offset;
   } else if (extras->data.format == AXVoiceFormat::LPCM8) {
      extras->data.endOffsetAbs = baseAddress + offset;
   } else {
      be_val<uint16_t> dataFormat = (uint16_t)extras->data.format;
      decaf_abort(fmt::format("Unexpected voice data format {}", dataFormat));
      //decaf_abort(fmt::format("Unexpected voice data type {}", extras->data.format));
   }

   extras->syncBits |= internal::AXVoiceSyncBits::EndOffset;
}

void
AXSetVoiceEndOffsetEx(AXVoice *voice,
                      uint32_t offset,
                      const void *samples)
{
   voice->offsets.data = samples;

   AXVoiceOffsets offsets;
   AXGetVoiceOffsets(voice, &offsets);

   AXSetVoiceEndOffset(voice, offset);
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
   auto extras = internal::getVoiceExtras(voice->index);

   voice->offsets.loopOffset = offset;

   auto baseAddress = voice->offsets.data.getAddress() & 0x1FFFFFFF;

   if (extras->data.format == AXVoiceFormat::ADPCM) {
      extras->data.loopOffsetAbs = (baseAddress << 1) + offset;
   } else if (extras->data.format == AXVoiceFormat::LPCM16) {
      extras->data.loopOffsetAbs = (baseAddress >> 1) + offset;
   } else if (extras->data.format == AXVoiceFormat::LPCM8) {
      extras->data.loopOffsetAbs = baseAddress + offset;
   } else {
      be_val<uint16_t> dataFormat = (uint16_t)extras->data.format;
      decaf_abort(fmt::format("Unexpected voice data format {}", dataFormat));
      //decaf_abort(fmt::format("Unexpected voice data type {}", extras->data.format));
   }

   extras->syncBits |= internal::AXVoiceSyncBits::LoopOffset;
}

void
AXSetVoiceLoopOffsetEx(AXVoice *voice,
                       uint32_t offset,
                       const void *samples)
{
   voice->offsets.data = samples;

   AXVoiceOffsets offsets;
   AXGetVoiceOffsets(voice, &offsets);

   AXSetVoiceLoopOffset(voice, offset);
}

void
AXSetVoiceLoop(AXVoice *voice,
               AXVoiceLoop loop)
{
   auto extras = internal::getVoiceExtras(voice->index);

   voice->offsets.loopingEnabled = loop;

   extras->data.loopFlag = loop;
   extras->syncBits |= internal::AXVoiceSyncBits::Loop;
}

uint32_t
AXSetVoiceMixerSelect(AXVoice *voice,
                      uint32_t mixerSelect)
{
   auto oldMiderSelect = voice->renderer;
   decaf_warn_stub();
   voice->renderer = static_cast<AXRenderer>(mixerSelect);
   return oldMiderSelect;
}

void
AXSetVoiceOffsets(AXVoice *voice,
                  AXVoiceOffsets *offsets)
{
   decaf_check(offsets->data);

   voice->offsets = *offsets;

   internal::AXCafeVoiceData absOffsets;

   absOffsets.format = offsets->dataType;
   absOffsets.loopFlag = offsets->loopingEnabled;
   auto samples = offsets->data.getAddress();

   if (offsets->dataType == AXVoiceFormat::ADPCM) {
      absOffsets.loopOffsetAbs = ((samples << 1) + offsets->loopOffset) & 0x3fffffff;
      absOffsets.endOffsetAbs = ((samples << 1) + offsets->endOffset) & 0x3fffffff;
      absOffsets.currentOffsetAbs = ((samples << 1) + offsets->currentOffset) & 0x3fffffff;
      absOffsets.memPageNumber = (samples + (offsets->currentOffset >> 1)) >> 29;
   } else if (offsets->dataType == AXVoiceFormat::LPCM16) {
      absOffsets.loopOffsetAbs = ((samples >> 1) + offsets->loopOffset) & 0x0fffffff;
      absOffsets.endOffsetAbs = ((samples >> 1) + offsets->endOffset) & 0x0fffffff;
      absOffsets.currentOffsetAbs = ((samples >> 1) + offsets->currentOffset) & 0x0fffffff;
      absOffsets.memPageNumber = (samples + (offsets->currentOffset << 1)) >> 29;
   } else if (offsets->dataType == AXVoiceFormat::LPCM8) {
      absOffsets.loopOffsetAbs = (samples + offsets->loopOffset) & 0x1fffffff;
      absOffsets.endOffsetAbs = (samples + offsets->endOffset) & 0x1fffffff;
      absOffsets.currentOffsetAbs = (samples + offsets->currentOffset) & 0x1fffffff;
      absOffsets.memPageNumber = (samples + offsets->currentOffset) >> 29;
   } else {
      be_val<uint16_t> offsetDataType = (uint16_t)offsets->dataType;
      decaf_abort(fmt::format("Unexpected voice data type {}", offsetDataType));
      //decaf_abort(fmt::format("Unexpected voice data type {}", offsets->dataType));
   }

   internal::setVoiceAddresses(voice, &absOffsets);
}

void
AXSetVoiceOffsetsEx(AXVoice *voice,
                    AXVoiceOffsets *offsets,
                    void *samples)
{
   auto adjOffsets = *offsets;
   AXSetVoiceOffsets(voice, &adjOffsets);
}

void
AXSetVoicePriority(AXVoice *voice,
                   uint32_t priority)
{
   voice->priority = priority;
}

void
AXSetVoiceRmtOn(AXVoice *voice,
                uint16_t on)
{
   decaf_warn_stub();
}

void
AXSetVoiceRmtIIRCoefs(AXVoice *voice,
                      uint16_t filter,
                      ppctypes::VarArgs)
{
   decaf_warn_stub();
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
   extras->src.ratio = ufixed1616_t { ratio };
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
   if (voice->state != state) {
      extras->state = state;
      voice->state = state;
      voice->syncBits |= internal::AXVoiceSyncBits::State;
   }
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

void
setVoiceAddresses(AXVoice *voice,
                  AXCafeVoiceData *offsets)
{
   auto extras = internal::getVoiceExtras(voice->index);

   extras->data = *offsets;

   if (offsets->format == AXVoiceFormat::ADPCM) {
      decaf_check((offsets->loopOffsetAbs & 0xF) >= 2);
      decaf_check((offsets->endOffsetAbs & 0xF) >= 2);
      decaf_check((offsets->currentOffsetAbs & 0xF) >= 2);
   } else if (offsets->format == AXVoiceFormat::LPCM8) {
      memset(&extras->adpcm, 0, sizeof(AXVoiceAdpcm));
      extras->adpcm.gain = 0x100;
      voice->syncBits |= internal::AXVoiceSyncBits::Adpcm;
   } else if (offsets->format == AXVoiceFormat::LPCM16) {
      memset(&extras->adpcm, 0, sizeof(AXVoiceAdpcm));
      extras->adpcm.gain = 0x800;
      voice->syncBits |= internal::AXVoiceSyncBits::Adpcm;
   } else {
      be_val<uint16_t> offsetFormat = (uint16_t)offsets->format;
      decaf_abort(fmt::format("Unexpected audio data format {}", offsetFormat));
      //decaf_abort(fmt::format("Unexpected audio data format {}", offsets->format));
   }

   voice->syncBits &= ~(
      internal::AXVoiceSyncBits::Loop |
      internal::AXVoiceSyncBits::LoopOffset |
      internal::AXVoiceSyncBits::EndOffset |
      internal::AXVoiceSyncBits::CurrentOffset);
   voice->syncBits |= internal::AXVoiceSyncBits::Addr;
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
   RegisterKernelFunction(AXGetVoiceMixerSelect);
   RegisterKernelFunction(AXGetVoiceOffsets);
   RegisterKernelFunction(AXGetVoiceOffsetsEx);
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
   RegisterKernelFunction(AXSetVoiceMixerSelect);
   RegisterKernelFunction(AXSetVoiceOffsets);
   RegisterKernelFunction(AXSetVoiceOffsetsEx);
   RegisterKernelFunction(AXSetVoicePriority);
   RegisterKernelFunction(AXSetVoiceRmtOn);
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
