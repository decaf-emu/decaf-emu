#include "snd_core.h"
#include "snd_core_device.h"
#include "snd_core_voice.h"
#include "decaf_sound.h"

namespace snd_core
{

static AXDeviceFinalMixCallback
gDeviceFinalMixCallback;

namespace internal
{

static int32_t
nextSampleAdpcm(AXVoice *voice,
                AXVoiceExtras *extras)
{
   auto data = reinterpret_cast<uint8_t *>(voice->offsets.data.get());

   if (voice->offsets.currentOffset % 16 == 0) {
      extras->adpcmPredScale = data[voice->offsets.currentOffset / 2];
      voice->offsets.currentOffset += 2;
   }

   auto sampleIndex = voice->offsets.currentOffset++;
   auto coeffIndex = (extras->adpcmPredScale >> 4) & 7;
   auto scale = extras->adpcmPredScale & 0xF;
   auto coeff0 = extras->adpcmCoeff[coeffIndex*2];
   auto coeff1 = extras->adpcmCoeff[coeffIndex*2 + 1];

   // Extract the 4-bit signed sample from the appropriate byte
   int sampleData = data[sampleIndex / 2];
   if (sampleIndex % 2 == 0) {
      sampleData &= 0xF;
   } else {
      sampleData >>= 4;
   }
   if (sampleData >= 8) {
      sampleData -= 16;
   }

   // Compute the output PCM sample value, clamp, and return
   int32_t prediction = ((extras->adpcmPrevSample[0] * coeff0)
                         + (extras->adpcmPrevSample[1] * coeff1)) >> 11;
   int32_t sample = prediction + (sampleData << scale);
   return std::min(std::max(sample, -32768), 32767);
}

static int32_t
nextSampleLpcm16(AXVoice *voice,
                  AXVoiceExtras *extras)
{
   auto data = reinterpret_cast<be_val<int16_t> *>(voice->offsets.data.get());
   return data[voice->offsets.currentOffset++];
}

static int32_t
nextSampleLpcm8(AXVoice *voice,
                AXVoiceExtras *extras)
{
   auto data = reinterpret_cast<uint8_t *>(voice->offsets.data.get());
   return (data[voice->offsets.currentOffset++] - 128) << 8;
}

// Returns false at end of stream (if not looped), else true
static bool
readNextSample(AXVoice *voice,
               AXVoiceExtras *extras)
{
   int32_t sample = 0;
   switch (voice->offsets.dataType) {
   case AXVoiceFormat::ADPCM:
      sample = nextSampleAdpcm(voice, extras);
      break;
   case AXVoiceFormat::LPCM16:
      sample = nextSampleLpcm16(voice, extras);
      break;
   case AXVoiceFormat::LPCM8:
      sample = nextSampleLpcm8(voice, extras);
      break;
   }

   extras->prevSample = extras->currentSample;
   extras->currentSample = sample;

   if (voice->offsets.currentOffset > voice->offsets.endOffset) {
      if (voice->offsets.loopingEnabled) {
         voice->offsets.currentOffset = voice->offsets.loopOffset;
         if (voice->offsets.dataType == AXVoiceFormat::ADPCM) {
            extras->adpcmPredScale = extras->adpcmLoopPredScale;
            extras->adpcmPrevSample[0] = extras->adpcmPrevSample[0];
            extras->adpcmPrevSample[1] = extras->adpcmPrevSample[1];
         }
         extras->loopCount++;
      } else {
         return false;
      }
   }

   return true;
}

void
mixOutput(int32_t *buffer, int numSamples, int numChannels)
{
   memset(buffer, 0, sizeof(*buffer) * numSamples * numChannels);

   const auto voices = getAcquiredVoices();

   for (auto voice : voices) {
      if (voice->state == AXVoiceState::Stopped) {
         continue;
      }

      auto extras = getVoiceExtras(voice->index);

      // TODO: This is all very inefficient at the moment.
      //  Plenty of room for improvement.
      for (auto i = 0; i < numSamples; ++i) {
         while (extras->offsetFrac >= 0) {
            extras->offsetFrac -= 0x10000;
            if (!readNextSample(voice, extras)) {
               voice->state = AXVoiceState::Stopped;
               break;
            }
         }
         auto weight = extras->offsetFrac + 0x10000;
         int32_t sample;
         if (weight == 0) {
            sample = extras->currentSample;
         } else {
            int32_t weightedPrev = extras->prevSample * (0x10000 - weight);
            int32_t weightedCurrent = extras->currentSample * weight;
            sample = ((weightedPrev + weightedCurrent) >> 16);
         }

         // TODO: figure out the "full volume" value
         const auto fullVolume = 0x8000;
         for (auto ch = 0; ch < numChannels; ++ch) {
            buffer[numChannels*i+ch] += sample * extras->tvVolume[ch] / fullVolume;
         }

         extras->offsetFrac += extras->playbackRatio;
      }
   }
}

} // namespace internal

AXResult
AXGetDeviceMode(AXDeviceType type, be_val<AXDeviceMode>* mode)
{
   // TODO: AXGetDeviceMode
   *mode = static_cast<AXDeviceMode>(0);
   return AXResult::Success;
}

AXResult
AXGetDeviceFinalMixCallback(AXDeviceType type, be_AXDeviceFinalMixCallback *func)
{
   if (func != nullptr) {
      *func = gDeviceFinalMixCallback;
   }

   return AXResult::Success;
}

AXResult
AXRegisterDeviceFinalMixCallback(AXDeviceType type, AXDeviceFinalMixCallback func)
{
   gDeviceFinalMixCallback = func;
   return AXResult::Success;
}

AXResult
AXGetAuxCallback(AXDeviceType type, uint32_t, uint32_t, be_AXAuxCallback *callback, be_ptr<void> *userData)
{
   // TODO: AXGetAuxCallback
   if (callback) {
      *callback = nullptr;
   }

   if (userData) {
      *userData = nullptr;
   }

   return AXResult::Success;
}

AXResult
AXRegisterAuxCallback(AXDeviceType type, uint32_t, uint32_t, AXAuxCallback callback, void *userData)
{
   // TODO: AXRegisterAuxCallback
   return AXResult::Success;
}

AXResult
AXSetDeviceLinearUpsampler(AXDeviceType type, uint32_t, uint32_t)
{
   // TODO: AXSetDeviceLinearUpsampler
   return AXResult::Success;
}

AXResult
AXSetDeviceCompressor(AXDeviceType type, uint32_t)
{
   // TODO: AXSetDeviceCompressor
   return AXResult::Success;
}

AXResult
AXSetDeviceUpsampleStage(AXDeviceType type, uint32_t)
{
   // TODO: AXSetDeviceUpsampleStage
   return AXResult::Success;
}

AXResult
AXSetDeviceVolume(AXDeviceType type, uint32_t id, uint16_t volume)
{
   return AXResult::Success;
}

void
Module::registerDeviceFunctions()
{
   RegisterKernelFunction(AXGetDeviceMode);
   RegisterKernelFunction(AXGetDeviceFinalMixCallback);
   RegisterKernelFunction(AXRegisterDeviceFinalMixCallback);
   RegisterKernelFunction(AXGetAuxCallback);
   RegisterKernelFunction(AXRegisterAuxCallback);
   RegisterKernelFunction(AXSetDeviceLinearUpsampler);
   RegisterKernelFunction(AXSetDeviceCompressor);
   RegisterKernelFunction(AXSetDeviceUpsampleStage);
   RegisterKernelFunction(AXSetDeviceVolume);
}

} // namespace snd_core
