#include "snd_core.h"
#include "snd_core_core.h"
#include "snd_core_constants.h"
#include "snd_core_device.h"
#include "snd_core_voice.h"
#include "decaf_sound.h"
#include "ppcutils/stackobject.h"
#include "ppcutils/wfunc_call.h"
#include <array>
#include <common/fixed.h>

namespace snd_core
{

struct PpcCallbackData
{
   be_ptr<be_val<int32_t>> samplePtrs[6];
   be_val<int32_t> samples[6][144];

   AXAuxCallbackData auxCallbackData;
   AXDeviceFinalMixData finalMixCallbackData;
};

static const int
NumOutputSamples = 48000 * 3 / 1000;

static const ufixed_1_15_t
DefaultVolume = ufixed_1_15_t::from_data(0x8000);

static PpcCallbackData *
sCallbackData = nullptr;

struct AuxData
{
   AXAuxCallback callback = nullptr;
   void *userData = nullptr;
   ufixed_1_15_t returnVolume = DefaultVolume;
};

struct DeviceData
{
   AXDeviceFinalMixCallback finalMixCallback = nullptr;
   std::array<AuxData, AXAuxId::Max> aux;
   bool linearUpsample = false;
   bool compressor = false;
   bool upsampleAfterFinalMix = false;
   ufixed_1_15_t volume = DefaultVolume;
   AXDeviceMode mode;
};

static std::array<DeviceData, AXNumTvDevices>
gTvDevices;

static std::array<DeviceData, AXNumDrcDevices>
gDrcDevices;

static std::array<DeviceData, AXNumRmtDevices>
gRmtDevices;

namespace internal
{

static DeviceData *
getDevice(AXDeviceType type,
          uint32_t deviceId)
{
   switch (type) {
   case AXDeviceType::TV:
      decaf_check(deviceId < gTvDevices.size());
      return &gTvDevices[deviceId];
   case AXDeviceType::DRC:
      decaf_check(deviceId < gDrcDevices.size());
      return &gDrcDevices[deviceId];
   case AXDeviceType::RMT:
      decaf_check(deviceId < gRmtDevices.size());
      return &gRmtDevices[deviceId];
   default:
      return nullptr;
   }
}

template<typename Type>
static Type*
getMemPageAddress(uint32_t memPageNumber)
{
   // We have to do this this way due to the way that mem::translate handles
   //  nullptr's.  In the case of AX here, our memPageNumber can be 0, causing
   //  mem::translate to return 0, which is not what we want.
   return reinterpret_cast<Type*>(mem::base() + (memPageNumber << 29));
}

struct AudioDecoder
{
   // Basic information
   internal::AXCafeVoiceData offsets;
   AXVoiceType type;
   uint32_t loopCount;
   AXVoiceAdpcm adpcm;
   AXVoiceAdpcmLoopData adpcmLoop;
   bool isEof;

   void fromVoice(AXVoiceExtras *extras)
   {
      offsets = extras->data;
      type = extras->type;
      loopCount = extras->loopCount;

      if (offsets.format == AXVoiceFormat::ADPCM) {
         adpcm = extras->adpcm;
         adpcmLoop = extras->adpcmLoop;
      }

      isEof = false;
   }

   void toVoice(AXVoiceExtras *extras)
   {
      extras->data = offsets;
      extras->loopCount = loopCount;

      if (offsets.format == AXVoiceFormat::ADPCM) {
         extras->adpcm = adpcm;
      }
   }

   AudioDecoder& advance()
   {
      // Update prev sample
      auto sample = read();
      adpcm.prevSample[1] = adpcm.prevSample[0];
      adpcm.prevSample[0] = sample.data();

      if (offsets.currentOffsetAbs == offsets.endOffsetAbs) {
         if (offsets.loopFlag) {
            offsets.currentOffsetAbs = offsets.loopOffsetAbs;

            if (offsets.format == AXVoiceFormat::ADPCM && type != AXVoiceType::Streaming) {
               adpcm.predScale = adpcmLoop.predScale;
               adpcm.prevSample[0] = adpcmLoop.prevSample[0];
               adpcm.prevSample[1] = adpcmLoop.prevSample[1];
            }
         } else {
            decaf_check(!isEof);
            isEof = true;
         }

         loopCount++;
      } else {
         offsets.currentOffsetAbs += 1;

         if (offsets.format == AXVoiceFormat::ADPCM) {
            // Read next header if were there
            if ((offsets.currentOffsetAbs & 0xf) < 2) {
               decaf_check((offsets.currentOffsetAbs & 0xf) == 0);

               auto data = getMemPageAddress<uint8_t>(offsets.memPageNumber);

               adpcm.predScale = data[offsets.currentOffsetAbs / 2];
               offsets.currentOffsetAbs += 2;
            }
         }
      }

      return *this;
   }

   bool eof()
   {
      return isEof;
   }

   Pcm16Sample read()
   {
      decaf_check(!isEof);

      auto sampleIndex = offsets.currentOffsetAbs;

      if (offsets.format == AXVoiceFormat::ADPCM) {
         decaf_check((sampleIndex & 0xf) >= 2);

         auto data = getMemPageAddress<uint8_t>(offsets.memPageNumber);

         auto coeffIndex = (adpcm.predScale.value() >> 4) & 7;
         auto scale = adpcm.predScale.value() & 0xF;
         auto yn1 = adpcm.prevSample[0].value();
         auto yn2 = adpcm.prevSample[1].value();
         auto coeff1 = adpcm.coefficients[coeffIndex * 2 + 0].value();
         auto coeff2 = adpcm.coefficients[coeffIndex * 2 + 1].value();

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

         // Calculate sample
         auto xn = sampleData << scale;
         auto adpcmSample = ((xn << 11) + 0x400 + (coeff1 * yn1) + (coeff2 * yn2)) >> 11;

         // Clamp the output
         auto clampedSample = std::min(std::max(adpcmSample, -32767), 32767);

         // Write to the output
         return Pcm16Sample::from_data(clampedSample);
      } else if (offsets.format == AXVoiceFormat::LPCM16) {
         auto data = getMemPageAddress<be_val<int16_t>>(offsets.memPageNumber);
         return Pcm16Sample::from_data(data[sampleIndex]);
      } else if (offsets.format == AXVoiceFormat::LPCM8) {
         auto data = getMemPageAddress<uint8_t>(offsets.memPageNumber);
         return Pcm16Sample::from_data(data[sampleIndex] << 8);
      } else {
         decaf_abort("Unexpected AXVoice data format");
      }
   }
};

void
sampleVoice(AXVoice *voice, Pcm16Sample *samples, int numSamples)
{
   static const auto FpOne = ufixed1616_t(1);
   static const auto FpZero = ufixed1616_t(0);

   memset(samples, 0, numSamples * sizeof(Pcm16Sample));

   auto extras = getVoiceExtras(voice->index);
   auto offsetFrac = ufixed1616_t(extras->src.currentOffsetFrac.value());

   AudioDecoder decoder;
   decoder.fromVoice(extras);

   for (auto i = 0; i < numSamples; ++i) {
      // Read in the current sample
      Pcm16Sample sample;
      if (offsetFrac == FpZero) {
         sample = decoder.read();
      } else {
         AudioDecoder nextDecoder(decoder);
         nextDecoder.advance();
         if (!nextDecoder.eof()) {
            sample = nextDecoder.read();
         } else {
            sample = 0;
         }
      }

      if (offsetFrac == FpZero) {
         samples[i] = sample;
      } else {
         auto thisSampleMul = FpOne - offsetFrac;
         auto lastSampleMul = offsetFrac;
         auto lastSample = Pcm16Sample::from_data(extras->src.lastSample[0]);
         samples[i] = sample * thisSampleMul + lastSample * lastSampleMul;
      }

      offsetFrac += extras->src.ratio;

      while (offsetFrac >= FpOne) {
         // Advance the voice by one sample
         offsetFrac -= FpOne;
         decoder.advance();

         // Update all the last sample listings.  Most of these are used
         //  for FFT resampling (which we don't currently handle).
         extras->src.lastSample[3] = extras->src.lastSample[2];
         extras->src.lastSample[2] = extras->src.lastSample[1];
         extras->src.lastSample[1] = extras->src.lastSample[0];
         extras->src.lastSample[0] = sample.data();

         // If we reached the end of the voice data, we should just leave
         if (decoder.eof()) {
            break;
         }

         // If we read through multiple samples due to a high SRC ratio,
         //  then we need to actually read the upcoming sample in expectation
         //  of the fact that its about to be stored in lastSample.
         if (offsetFrac >= FpOne) {
            sample = decoder.read();
         }
      }

      if (decoder.eof()) {
         break;
      }
   }

   if (decoder.eof()) {
      voice->state = AXVoiceState::Stopped;
   }

   decoder.toVoice(extras);

   extras->src.currentOffsetFrac = ufixed1616_t(offsetFrac);
}

void
decodeVoiceSamples(int numSamples)
{
   const auto voices = getAcquiredVoices();

   for (auto voice : voices) {
      auto extras = getVoiceExtras(voice->index);

      if (voice->state == AXVoiceState::Stopped) {
         extras->numSamples = 0;
         continue;
      }

      extras->numSamples = numSamples;
      sampleVoice(voice, extras->samples, numSamples);
   }

   // TODO: Apply Volume Evelope (ADSR)

   // TODO: Apply Biquad Filter

   // TODO: Apply Low Pass Filter
}

static Pcm16Sample gTvSamples[AXNumTvDevices][AXNumTvChannels][144];

void
invokeAuxCallback(AuxData &aux, uint32_t numChannels, uint32_t numSamples, Pcm16Sample samples[6][144])
{
   if (aux.callback) {
      auto auxCbData = &sCallbackData->auxCallbackData;
      auxCbData->samples = numSamples;
      auxCbData->channels = numChannels;

      for (auto ch = 0u; ch < numChannels; ++ch) {
         for (auto i = 0u; i < numSamples; ++i) {
            sCallbackData->samples[ch][i] = static_cast<int32_t>(samples[ch][i]);
         }
         sCallbackData->samplePtrs[ch] = &sCallbackData->samples[ch][0];
      }

      aux.callback(sCallbackData->samplePtrs, aux.userData, auxCbData);

      for (auto ch = 0u; ch < numChannels; ++ch) {
         for (auto i = 0u; i < numSamples; ++i) {
            samples[ch][i] = Pcm16Sample::from_data(sCallbackData->samples[ch][i]);
         }
      }
   }
}

void
invokeFinalMixCallback(DeviceData &device, uint32_t numChannels, uint32_t numSamples, Pcm16Sample samples[6][144])
{
   if (device.finalMixCallback) {
      auto mixCbData = &sCallbackData->finalMixCallbackData;
      mixCbData->channels = numChannels;
      mixCbData->samples = numSamples;
      mixCbData->unk1 = 1;
      mixCbData->channelsOut = mixCbData->channels;

      for (auto ch = 0u; ch < numChannels; ++ch) {
         for (auto i = 0u; i < numSamples; ++i) {
            sCallbackData->samples[ch][i] = static_cast<int32_t>(samples[ch][i]);
         }
         sCallbackData->samplePtrs[ch] = &sCallbackData->samples[ch][0];
      }
      mixCbData->data = sCallbackData->samplePtrs;

      device.finalMixCallback(mixCbData);

      for (auto ch = 0u; ch < numChannels; ++ch) {
         for (auto i = 0u; i < numSamples; ++i) {
            samples[ch][i] = Pcm16Sample::from_data(sCallbackData->samples[ch][i]);
         }
      }
   }
}

AXVoiceExtras::MixVolume &
getVoiceMixVolume(AXVoiceExtras *extras, AXDeviceType type, uint32_t device, uint32_t channel, uint32_t bus)
{
   if (type == AXDeviceType::TV) {
      decaf_check(device < AXNumTvDevices);
      return extras->tvVolume[device][bus][channel];
   } else if (type == AXDeviceType::DRC) {
      decaf_check(device < AXNumDrcDevices);
      return extras->drcVolume[device][bus][channel];
   } else if (type == AXDeviceType::RMT) {
      decaf_check(device < AXNumRmtDevices);
      return extras->rmtVolume[device][bus][channel];
   } else {
      decaf_abort("Unexpected device type");
   }
}

uint32_t
getDeviceNumBuses(AXDeviceType type)
{
   decaf_check(type < AXDeviceType::Max);
   static const uint32_t busses[] = { 4, 4, 1 };
   return busses[type];
}

uint32_t
getDeviceNumChannels(AXDeviceType type)
{
   decaf_check(type < AXDeviceType::Max);
   static const uint32_t channels[] = {6, 4, 1};
   return channels[type];
}

void
mixDevice(AXDeviceType type, uint32_t deviceId, uint32_t numSamples)
{
   auto device = getDevice(type, deviceId);
   auto numBus = getDeviceNumBuses(type);
   auto numChannels = getDeviceNumChannels(type);

   decaf_check(numBus <= 4);
   decaf_check(numChannels <= 6);
   decaf_check(numSamples == 96 || numSamples == 144);

   Pcm16Sample busSamples[4][6][144];
   const auto voices = getAcquiredVoices();

   memset(busSamples, 0, sizeof(Pcm16Sample) * 4 * 6 * 144);

   for (auto voice : voices) {
      auto extras = getVoiceExtras(voice->index);

      if (!extras->numSamples) {
         continue;
      }

      decaf_check(extras->numSamples == numSamples);

      for (auto bus = 0u; bus < numBus; ++bus) {
         for (auto channel = 0u; channel < numChannels; ++channel) {
            auto &volume = getVoiceMixVolume(extras, type, deviceId, channel, bus);
            auto &out = busSamples[bus][channel];

            for (auto i = 0u; i < numSamples; ++i) {
               out[i] += extras->samples[i] * volume.volume;
            }

            volume.volume += volume.delta;
         }
      }
   }

   for (auto bus = 1u; bus < numBus; ++bus) {
      invokeAuxCallback(device->aux[bus - 1], numChannels, numSamples, busSamples[bus]);
   }

   auto &mainBus = busSamples[0];

   // Downmix all aux busses to main bus
   for (auto bus = 1u; bus < numBus; ++bus) {
      auto returnVolume = device->aux[bus].returnVolume;
      auto subBus = busSamples[bus];

      for (auto channel = 0u; channel < numChannels; ++channel) {
         for (auto i = 0u; i < numSamples; ++i) {
            mainBus[channel][i] += subBus[channel][i] * returnVolume;
         }
      }
   }

   // Apply overall device volume
   for (auto channel = 0u; channel < numChannels; ++channel) {
      for (auto i = 0u; i < numSamples; ++i) {
         mainBus[channel][i] = mainBus[channel][i] * device->volume;
      }
   }

   // Now we need to perform upsampling (I think)
   auto upsample32to48 = [](Pcm16Sample *samples) {
      // currently lazy...
      auto output = samples;
      Pcm16Sample input[96];
      memcpy(input, output, sizeof(Pcm16Sample) * 96);

      // Perform upsampling
      for (auto i = 0u; i < NumOutputSamples; ++i) {
         float sampleIdx = static_cast<float>(i) / 144.0f * 96.0f;
         auto sampleLo = static_cast<uint32_t>(std::min(143.0f, std::floor(sampleIdx)));
         auto sampleHi = static_cast<uint32_t>(std::min(95.0f, std::ceil(sampleIdx)));
         float sampleFrac = sampleIdx - sampleLo;
         output[i] = (input[sampleLo] * (1.0f - sampleFrac)) + (input[sampleHi] * sampleFrac);
      }
   };

   // Perform upsampling and final mix callback invokation
   if (device->upsampleAfterFinalMix) {
      invokeFinalMixCallback(*device, numChannels, numSamples, mainBus);

      if (numSamples != NumOutputSamples) {
         for (auto channel = 0u; channel < numChannels; ++channel) {
            upsample32to48(mainBus[channel]);
         }
      }
   } else {
      if (numSamples != NumOutputSamples) {
         for (auto channel = 0u; channel < numChannels; ++channel) {
            upsample32to48(mainBus[channel]);
         }
      }

      invokeFinalMixCallback(*device, numChannels, numSamples, mainBus);
   }

   // TODO: Apply compressor

   // TODO: Channel upmix/downmix, but I think we should let the audio driver (aka SDL) handle that

   if (type == AXDeviceType::TV) {
      // Copy the generated data out for later pickup
      memcpy(&gTvSamples[deviceId][0][0], &mainBus[0][0], sizeof(Pcm16Sample) * numChannels * NumOutputSamples);
   } else if (type == AXDeviceType::DRC) {
      // We currently just discard the generated DRC audio
   } else if (type == AXDeviceType::RMT) {
      // We also discard generated RMT audio
   } else {
      decaf_abort("Unexpected device type during copy-out");
   }
}

void
mixOutput(int32_t *buffer, int numSamples, int numChannels)
{
   static const int NumOutputSamples = 48000 * 3 / 1000;

   // Decode audio samples from the source voices
   decodeVoiceSamples(numSamples);

   // Mix all the TV devices
   for (auto deviceId = 0; deviceId < AXNumTvDevices; ++deviceId) {
      mixDevice(AXDeviceType::TV, deviceId, numSamples);
   }

   // Mix all the DRC devices
   for (auto deviceId = 0; deviceId < AXNumDrcDevices; ++deviceId) {
      mixDevice(AXDeviceType::DRC, deviceId, numSamples);
   }

   // Mix all the RMT devices
   for (auto deviceId = 0; deviceId < AXNumRmtDevices; ++deviceId) {
      mixDevice(AXDeviceType::RMT, deviceId, numSamples);
   }

   // Send off the TV device 0 data to be played on host
   for (auto i = 0; i < NumOutputSamples; ++i) {
      for (auto ch = 0; ch < numChannels; ++ch) {
         buffer[numChannels * i + ch] = gTvSamples[0][ch][i].data();
      }
   }
}

} // namespace internal

AXResult
AXGetDeviceMode(AXDeviceType type,
                be_val<AXDeviceMode> *mode)
{
   if (!mode) {
      return AXResult::Success;
   }

   switch (type) {
   case AXDeviceType::TV:
      *mode = gTvDevices[0].mode;
      break;
   case AXDeviceType::DRC:
      *mode = gDrcDevices[0].mode;
      break;
   case AXDeviceType::RMT:
      *mode = gRmtDevices[0].mode;
      break;
   default:
      return AXResult::InvalidDeviceType;
   }

   return AXResult::Success;
}

AXResult
AXSetDeviceMode(AXDeviceType type,
                AXDeviceMode mode)
{
   switch (type) {
   case AXDeviceType::TV:
      for (auto &device : gTvDevices) {
         device.mode = mode;
      }
      break;
   case AXDeviceType::DRC:
      for (auto &device : gDrcDevices) {
         device.mode = mode;
      }
      break;
   case AXDeviceType::RMT:
      for (auto &device : gRmtDevices) {
         device.mode = mode;
      }
      break;
   default:
      return AXResult::InvalidDeviceType;
   }

   return AXResult::Success;
}

AXResult
AXGetDeviceFinalMixCallback(AXDeviceType type,
                            be_AXDeviceFinalMixCallback *func)
{
   if (!func) {
      return AXResult::Success;
   }

   switch (type) {
   case AXDeviceType::TV:
      *func= gTvDevices[0].finalMixCallback;
      break;
   case AXDeviceType::DRC:
      *func = gDrcDevices[0].finalMixCallback;
      break;
   case AXDeviceType::RMT:
      *func = gRmtDevices[0].finalMixCallback;
      break;
   default:
      return AXResult::InvalidDeviceType;
   }

   return AXResult::Success;
}

AXResult
AXRegisterDeviceFinalMixCallback(AXDeviceType type,
                                 AXDeviceFinalMixCallback func)
{
   switch (type) {
   case AXDeviceType::TV:
      for (auto &device : gTvDevices) {
         device.finalMixCallback = func;
      }
      break;
   case AXDeviceType::DRC:
      for (auto &device : gDrcDevices) {
         device.finalMixCallback = func;
      }
      break;
   case AXDeviceType::RMT:
      for (auto &device : gRmtDevices) {
         device.finalMixCallback = func;
      }
      break;
   default:
      return AXResult::InvalidDeviceType;
   }

   return AXResult::Success;
}

AXResult
AXGetAuxCallback(AXDeviceType type,
                 uint32_t deviceId,
                 AXAuxId auxId,
                 be_AXAuxCallback *callback,
                 be_ptr<void> *userData)
{
   auto device = internal::getDevice(type, deviceId);

   if (!device) {
      return AXResult::InvalidDeviceType;
   }

   if (callback) {
      *callback = device->aux[auxId].callback;
   }

   if (userData) {
      *userData = device->aux[auxId].userData;
   }

   return AXResult::Success;
}

AXResult
AXRegisterAuxCallback(AXDeviceType type,
                      uint32_t deviceId,
                      AXAuxId auxId,
                      AXAuxCallback callback,
                      void *userData)
{
   auto device = internal::getDevice(type, deviceId);

   if (!device) {
      return AXResult::InvalidDeviceType;
   }

   device->aux[auxId].callback = callback;
   device->aux[auxId].userData = userData;

   return AXResult::Success;
}

AXResult
AXSetDeviceLinearUpsampler(AXDeviceType type,
                           uint32_t deviceId,
                           BOOL linear)
{
   auto device = internal::getDevice(type, deviceId);

   if (!device) {
      return AXResult::InvalidDeviceType;
   }

   device->linearUpsample = !!linear;
   return AXResult::Success;
}

AXResult
AXSetDeviceCompressor(AXDeviceType type,
                      BOOL compressor)
{
   switch (type) {
   case AXDeviceType::TV:
      for (auto &device : gTvDevices) {
         device.compressor = !!compressor;
      }
      break;
   case AXDeviceType::DRC:
      for (auto &device : gDrcDevices) {
         device.compressor = !!compressor;
      }
      break;
   case AXDeviceType::RMT:
      for (auto &device : gRmtDevices) {
         device.compressor = !!compressor;
      }
      break;
   default:
      return AXResult::InvalidDeviceType;
   }

   return AXResult::Success;
}

AXResult
AXGetDeviceUpsampleStage(AXDeviceType type,
                         BOOL *upsampleAfterFinalMixCallback)
{
   if (!upsampleAfterFinalMixCallback) {
      return AXResult::Success;
   }

   switch (type) {
   case AXDeviceType::TV:
      *upsampleAfterFinalMixCallback = gTvDevices[0].upsampleAfterFinalMix ? TRUE : FALSE;
      break;
   case AXDeviceType::DRC:
      *upsampleAfterFinalMixCallback = gDrcDevices[0].upsampleAfterFinalMix ? TRUE : FALSE;
      break;
   case AXDeviceType::RMT:
      *upsampleAfterFinalMixCallback = gRmtDevices[0].upsampleAfterFinalMix ? TRUE : FALSE;
      break;
   default:
      return AXResult::InvalidDeviceType;
   }

   return AXResult::Success;
}

AXResult
AXSetDeviceUpsampleStage(AXDeviceType type,
                         BOOL upsampleAfterFinalMixCallback)
{
   switch (type) {
   case AXDeviceType::TV:
      for (auto &device : gTvDevices) {
         device.upsampleAfterFinalMix = !!upsampleAfterFinalMixCallback;
      }
      break;
   case AXDeviceType::DRC:
      for (auto &device : gDrcDevices) {
         device.upsampleAfterFinalMix = !!upsampleAfterFinalMixCallback;
      }
      break;
   case AXDeviceType::RMT:
      for (auto &device : gRmtDevices) {
         device.upsampleAfterFinalMix = !!upsampleAfterFinalMixCallback;
      }
      break;
   default:
      return AXResult::InvalidDeviceType;
   }

   return AXResult::Success;
}

AXResult
AXGetDeviceVolume(AXDeviceType type,
                  uint32_t deviceId,
                  be_val<uint16_t> *volume)
{
   auto device = internal::getDevice(type, deviceId);

   if (!device) {
      return AXResult::InvalidDeviceType;
   }

   if (volume) {
      *volume = device->volume.data();
   }

   return AXResult::Success;
}

AXResult
AXSetDeviceVolume(AXDeviceType type,
                  uint32_t deviceId,
                  uint16_t volume)
{
   auto device = internal::getDevice(type, deviceId);

   if (!device) {
      return AXResult::InvalidDeviceType;
   }

   device->volume = ufixed_1_15_t::from_data(volume);
   return AXResult::Success;
}

AXResult
AXGetAuxReturnVolume(AXDeviceType type,
                     uint32_t deviceId,
                     AXAuxId auxId,
                     be_val<uint16_t> *volume)
{
   auto device = internal::getDevice(type, deviceId);

   if (!device) {
      return AXResult::InvalidDeviceType;
   }

   if (volume) {
      *volume = device->aux[auxId].returnVolume.data();
   }

   return AXResult::Success;
}

AXResult
AXSetAuxReturnVolume(AXDeviceType type,
                     uint32_t deviceId,
                     AXAuxId auxId,
                     uint16_t volume)
{
   auto device = internal::getDevice(type, deviceId);

   if (!device) {
      return AXResult::InvalidDeviceType;
   }

   device->aux[auxId].returnVolume = ufixed_1_15_t::from_data(volume);
   return AXResult::Success;
}

void
Module::registerDeviceFunctions()
{
   RegisterKernelFunction(AXGetDeviceMode);
   RegisterKernelFunction(AXSetDeviceMode);
   RegisterKernelFunction(AXGetDeviceFinalMixCallback);
   RegisterKernelFunction(AXRegisterDeviceFinalMixCallback);
   RegisterKernelFunction(AXGetAuxCallback);
   RegisterKernelFunction(AXRegisterAuxCallback);
   RegisterKernelFunction(AXSetDeviceLinearUpsampler);
   RegisterKernelFunction(AXSetDeviceCompressor);
   RegisterKernelFunction(AXGetDeviceUpsampleStage);
   RegisterKernelFunction(AXSetDeviceUpsampleStage);
   RegisterKernelFunction(AXGetDeviceVolume);
   RegisterKernelFunction(AXSetDeviceVolume);
   RegisterKernelFunction(AXGetAuxReturnVolume);
   RegisterKernelFunction(AXSetAuxReturnVolume);

   RegisterInternalData(sCallbackData);
}

} // namespace snd_core
