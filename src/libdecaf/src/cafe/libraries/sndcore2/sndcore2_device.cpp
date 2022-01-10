#include "sndcore2.h"
#include "sndcore2_config.h"
#include "sndcore2_constants.h"
#include "sndcore2_device.h"
#include "sndcore2_voice.h"
#include "decaf_sound.h"

#include "cafe/cafe_ppc_interface_invoke_guest.h"

#include <array>
#include <common/fixed.h>
#include <libcpu/mmu.h>

namespace cafe::sndcore2
{

constexpr auto NumOutputSamples = 48000 * 3 / 1000;
constexpr auto DefaultVolume = ufixed_1_15_t { 1.0 };

struct AuxData
{
   AXAuxCallback callback;
   virt_ptr<void> userData;
   ufixed_1_15_t returnVolume;
};

struct DeviceData
{
   std::array<AuxData, AXAuxId::Max> aux;
   bool linearUpsample;
   ufixed_1_15_t volume;
};

struct DeviceTypeData
{
   std::array<DeviceData, 4> devices;
   bool compressor;
   AXDeviceFinalMixCallback finalMixCallback;
   bool upsampleAfterFinalMix;
   AXDeviceMode mode;
};

struct StaticDeviceData
{
   be2_struct<DeviceTypeData> tvDevices;
   be2_struct<DeviceTypeData> drcDevices;
   be2_struct<DeviceTypeData> rmtDevices;

   be2_array<virt_ptr<int32_t>, 8> samplePtrs;
   be2_val<int32_t> samples[8][144];
   be2_struct<AXAuxCallbackData> auxCallbackData;
   be2_struct<AXDeviceFinalMixData> finalMixCallbackData;
};

static virt_ptr<StaticDeviceData>
sDeviceData = nullptr;

namespace internal
{

static DeviceData *
getDevice(AXDeviceType type,
          uint32_t deviceId)
{
   switch (type) {
   case AXDeviceType::TV:
      decaf_check(deviceId < AXNumTvDevices);
      return &sDeviceData->tvDevices.devices[deviceId];
   case AXDeviceType::DRC:
      decaf_check(deviceId < AXNumDrcDevices);
      return &sDeviceData->drcDevices.devices[deviceId];
   case AXDeviceType::RMT:
      decaf_check(deviceId < AXNumRmtDevices);
      return &sDeviceData->rmtDevices.devices[deviceId];
   default:
      return nullptr;
   }
}

template<typename Type>
static Type *
getMemPageAddress(uint32_t memPageNumber)
{
   // We have to do this this way due to the way that mem::translate handles
   //  nullptr's.  In the case of AX here, our memPageNumber can be 0, causing
   //  mem::translate to return 0, which is not what we want.
   return reinterpret_cast<Type *>(cpu::getBaseVirtualAddress() + (static_cast<uint64_t>(memPageNumber) << 29));
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

   void fromVoice(virt_ptr<AXVoiceExtras> extras)
   {
      offsets = extras->data;
      type = extras->type;
      loopCount = extras->loopCount;
      adpcm = extras->adpcm;
      adpcmLoop = extras->adpcmLoop;

      isEof = false;
   }

   void toVoice(virt_ptr<AXVoiceExtras> extras)
   {
      extras->data = offsets;
      extras->loopCount = loopCount;
      extras->adpcm = adpcm;
   }

   AudioDecoder& advance()
   {
      // Update prev sample
      auto sample = read();
      adpcm.prevSample[1] = adpcm.prevSample[0];
      adpcm.prevSample[0] = fixed_to_data(sample);

      if (offsets.currentOffsetAbs == offsets.endOffsetAbs) {
         // According to Dolphin, the loop back happens regardless
         //  of whether the voice is in looping mode
         offsets.currentOffsetAbs = offsets.loopOffsetAbs;

         if (offsets.loopFlag) {
            adpcm.predScale = adpcmLoop.predScale;

            if (type != AXVoiceType::Streaming) {
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
            if ((offsets.currentOffsetAbs & 0xf) < virt_addr { 2 }) {
               decaf_check((offsets.currentOffsetAbs & 0xf) == virt_addr { 0 });

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
      auto sampleIndex = static_cast<uint32_t>(offsets.currentOffsetAbs);

      if (offsets.format == AXVoiceFormat::ADPCM) {
         decaf_check((sampleIndex & 0xf) >= 2);

         auto data = getMemPageAddress<uint8_t>(offsets.memPageNumber);

         auto scale = 1 << (adpcm.predScale.value() & 0xF);
         auto coeffIndex = (adpcm.predScale.value() >> 4) & 7;
         auto coeff1 = adpcm.coefficients[coeffIndex * 2 + 0].value();
         auto coeff2 = adpcm.coefficients[coeffIndex * 2 + 1].value();
         auto yn1 = adpcm.prevSample[0].value();
         auto yn2 = adpcm.prevSample[1].value();

         // Extract the 4-bit signed sample from the appropriate byte
         int sampleData = data[sampleIndex / 2];

         if (sampleIndex % 2 == 0) {
            sampleData >>= 4;
         } else {
            sampleData &= 0xF;
         }

         if (sampleData >= 8) {
            sampleData -= 16;
         }

         // Calculate sample
         auto adpcmSample = (scale * sampleData << 11) + ((0x400 + (coeff1 * yn1) + (coeff2 * yn2)) >> 11);

         // Clamp the output
         auto clampedSample = std::min(std::max(adpcmSample, -32767), 32767);

         // Write to the output
         return fixed_from_data<Pcm16Sample>(static_cast<int16_t>(clampedSample));
      } else if (offsets.format == AXVoiceFormat::LPCM16) {
         auto data = getMemPageAddress<be2_val<int16_t>>(offsets.memPageNumber);
         return fixed_from_data<Pcm16Sample>(data[sampleIndex]);
      } else if (offsets.format == AXVoiceFormat::LPCM8) {
         auto data = getMemPageAddress<uint8_t>(offsets.memPageNumber);
         return fixed_from_data<Pcm16Sample>(static_cast<int16_t>(data[sampleIndex] << 8));
      } else {
         decaf_abort("Unexpected AXVoice data format");
      }
   }
};

void
sampleVoice(virt_ptr<AXVoice> voice,
            Pcm16Sample *samples,
            int numSamples)
{
   static const auto FpOne = ufixed_16_16_t(1);
   static const auto FpZero = ufixed_16_16_t(0);

   memset(samples, 0, numSamples * sizeof(Pcm16Sample));

   auto extras = getVoiceExtras(voice->index);
   auto offsetFrac = ufixed_16_16_t(extras->src.currentOffsetFrac.value());

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
         auto lastSample = fixed_from_data<Pcm16Sample>(extras->src.lastSample[0]);
         samples[i] = sample * thisSampleMul + lastSample * lastSampleMul;
      }

      offsetFrac += extras->src.ratio.value();

      while (offsetFrac >= FpOne) {
         // Advance the voice by one sample
         offsetFrac -= FpOne;
         decoder.advance();

         // Update all the last sample listings.  Most of these are used
         //  for FFT resampling (which we don't currently handle).
         extras->src.lastSample[3] = extras->src.lastSample[2];
         extras->src.lastSample[2] = extras->src.lastSample[1];
         extras->src.lastSample[1] = extras->src.lastSample[0];
         extras->src.lastSample[0] = fixed_to_data(sample);

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

   extras->src.currentOffsetFrac = ufixed_0_16_t { offsetFrac };
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

static Pcm16Sample gTvSamples[AXNumTvDevices][AXNumTvChannels][NumOutputSamples];

static void
invokeAuxCallback(AuxData &aux, uint32_t numChannels, uint32_t numSamples, Pcm16Sample samples[6][144])
{
   if (aux.callback) {
      auto auxCbData = virt_addrof(sDeviceData->auxCallbackData);
      auxCbData->samples = numSamples;
      auxCbData->channels = numChannels;

      for (auto ch = 0u; ch < numChannels; ++ch) {
         for (auto i = 0u; i < numSamples; ++i) {
            sDeviceData->samples[ch][i] = static_cast<int32_t>(samples[ch][i]);
         }

         sDeviceData->samplePtrs[ch] = virt_addrof(sDeviceData->samples[ch][0]);
      }

      cafe::invoke(cpu::this_core::state(),
                   aux.callback,
                   virt_addrof(sDeviceData->samplePtrs),
                   aux.userData,
                   auxCbData);

      for (auto ch = 0u; ch < numChannels; ++ch) {
         for (auto i = 0u; i < numSamples; ++i) {
            samples[ch][i] = fixed_from_data<Pcm16Sample>(
               static_cast<int16_t>(sDeviceData->samples[ch][i]));
         }
      }
   }
}

static void
invokeFinalMixCallback(DeviceTypeData &device,
                       uint16_t numDevices,
                       uint16_t numChannels,
                       uint16_t numSamples,
                       Pcm16Sample samples[4][6][144])
{
   if (device.finalMixCallback) {
      auto mixCbData = virt_addrof(sDeviceData->finalMixCallbackData);
      mixCbData->channels = numChannels;
      mixCbData->samples = numSamples;
      mixCbData->numDevices = numDevices;
      mixCbData->channelsOut = mixCbData->channels;

      for (auto dev = 0u; dev < numDevices; ++dev) {
         for (auto ch = 0u; ch < numChannels; ++ch) {
            auto axChanId = (dev * numChannels) + ch;

            for (auto i = 0u; i < numSamples; ++i) {
               int16_t sample = fixed_to_data(samples[dev][ch][i]);
               sDeviceData->samples[axChanId][i] = static_cast<int32_t>(sample);
            }

            sDeviceData->samplePtrs[axChanId] = virt_addrof(sDeviceData->samples[axChanId][0]);
         }
      }
      mixCbData->data = virt_addrof(sDeviceData->samplePtrs);

      cafe::invoke(cpu::this_core::state(),
                   device.finalMixCallback,
                   mixCbData);

      for (auto dev = 0u; dev < numDevices; ++dev) {
         for (auto ch = 0u; ch < numChannels; ++ch) {
            auto axChanId = (dev * numChannels) + ch;

            for (auto i = 0u; i < numSamples; ++i) {
               samples[dev][ch][i] = fixed_from_data<Pcm16Sample>(
                  static_cast<int16_t>(sDeviceData->samples[axChanId][i]));
            }
         }
      }
   }
}

AXVoiceExtras::MixVolume &
getVoiceMixVolume(virt_ptr<AXVoiceExtras> extras,
                  AXDeviceType type,
                  uint32_t device,
                  uint32_t channel,
                  uint32_t bus)
{
   if (type == AXDeviceType::TV) {
      decaf_check(device < AXNumTvDevices);
      decaf_check(channel < AXNumTvChannels);
      decaf_check(bus < AXNumTvBus);
      return extras->tvVolume[device][channel][bus];
   } else if (type == AXDeviceType::DRC) {
      decaf_check(device < AXNumDrcDevices);
      decaf_check(channel < AXNumDrcChannels);
      decaf_check(bus < AXNumDrcBus);
      return extras->drcVolume[device][channel][bus];
   } else if (type == AXDeviceType::RMT) {
      decaf_check(device < AXNumRmtDevices);
      decaf_check(channel < AXNumRmtChannels);
      decaf_check(bus < AXNumRmtBus);
      return extras->rmtVolume[device][channel][bus];
   } else {
      decaf_abort("Unexpected device type");
   }
}

static virt_ptr<DeviceTypeData>
getDeviceGroup(AXDeviceType type)
{
   switch (type) {
   case AXDeviceType::TV:
      return virt_addrof(sDeviceData->tvDevices);
   case AXDeviceType::DRC:
      return virt_addrof(sDeviceData->drcDevices);
   case AXDeviceType::RMT:
      return virt_addrof(sDeviceData->rmtDevices);
   default:
      decaf_abort("Unexpected device type");
   }
}

static uint16_t
getDeviceNumDevices(AXDeviceType type)
{
   decaf_check(type < AXDeviceType::Max);
   static const uint16_t devices[] = { AXNumTvDevices, AXNumDrcDevices, AXNumRmtDevices };
   return devices[type];
}

static uint16_t
getDeviceNumBuses(AXDeviceType type)
{
   decaf_check(type < AXDeviceType::Max);
   static const uint16_t busses[] = { AXNumTvBus, AXNumDrcBus, AXNumRmtBus };
   return busses[type];
}

static uint16_t
getDeviceNumChannels(AXDeviceType type)
{
   decaf_check(type < AXDeviceType::Max);
   static const uint16_t channels[] = { AXNumTvChannels, AXNumDrcChannels, AXNumRmtChannels };
   return channels[type];
}

static void
mixDevice(AXDeviceType type, uint16_t numSamples)
{
   static const auto AXMaxDevices = 4;
   static const auto AXMaxBuses = 4;
   static const auto AXMaxChannels = 6;

   auto devices = getDeviceGroup(type);
   auto numDevices = getDeviceNumDevices(type);
   auto numBus = getDeviceNumBuses(type);
   auto numChannels = getDeviceNumChannels(type);

   decaf_check(numDevices <= AXMaxDevices);
   decaf_check(numBus <= AXMaxBuses);
   decaf_check(numChannels <= AXMaxChannels);
   decaf_check(numSamples == 96 || numSamples == 144);

   Pcm16Sample busSamples[AXMaxBuses][AXMaxDevices][AXMaxChannels][NumOutputSamples];
   const auto voices = getAcquiredVoices();

   memset(busSamples, 0, sizeof(Pcm16Sample) * AXMaxBuses * AXMaxDevices * AXMaxChannels * NumOutputSamples);

   for (auto voice : voices) {
      auto extras = getVoiceExtras(voice->index);

      if (!extras->numSamples) {
         continue;
      }

      decaf_check(extras->numSamples == numSamples);

      for (auto deviceId = 0u; deviceId < numDevices; ++deviceId) {
         for (auto bus = 0u; bus < numBus; ++bus) {
            for (auto channel = 0u; channel < numChannels; ++channel) {
               auto &volume = getVoiceMixVolume(extras, type, deviceId, channel, bus);
               auto &out = busSamples[bus][deviceId][channel];

               for (auto i = 0u; i < numSamples; ++i) {
                  out[i] += extras->samples[i] * volume.volume;
               }

               volume.volume += volume.delta;
            }
         }
      }
   }

   for (auto deviceId = 0u; deviceId < numDevices; ++deviceId) {
      auto &device = devices->devices[deviceId];

      for (auto bus = 1u; bus < numBus; ++bus) {
         invokeAuxCallback(device.aux[bus - 1], numChannels, numSamples, busSamples[bus][deviceId]);
      }
   }

   auto &mainBus = busSamples[0];

   // Downmix all aux busses to main bus
   for (auto deviceId = 0u; deviceId < numDevices; ++deviceId) {
      auto &device = devices->devices[deviceId];

      for (auto bus = 1u; bus < numBus; ++bus) {
         auto returnVolume = device.aux[bus - 1].returnVolume;
         auto subBus = busSamples[bus];

         for (auto channel = 0u; channel < numChannels; ++channel) {
            for (auto i = 0u; i < numSamples; ++i) {
               mainBus[deviceId][channel][i] += subBus[deviceId][channel][i] * returnVolume;
            }
         }
      }
   }

   // Apply overall device volume
   for (auto deviceId = 0u; deviceId < numDevices; ++deviceId) {
      auto &device = devices->devices[deviceId];

      for (auto channel = 0u; channel < numChannels; ++channel) {
         for (auto i = 0u; i < numSamples; ++i) {
            mainBus[deviceId][channel][i] = mainBus[deviceId][channel][i] * device.volume;
         }
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
   if (devices->upsampleAfterFinalMix) {
      invokeFinalMixCallback(*devices, numDevices, numChannels, numSamples, mainBus);

      if (numSamples != NumOutputSamples) {
         for (auto deviceId = 0u; deviceId < numDevices; ++deviceId) {
            for (auto channel = 0u; channel < numChannels; ++channel) {
               upsample32to48(mainBus[deviceId][channel]);
            }
         }
      }
   } else {
      if (numSamples != NumOutputSamples) {
         for (auto deviceId = 0u; deviceId < numDevices; ++deviceId) {
            for (auto channel = 0u; channel < numChannels; ++channel) {
               upsample32to48(mainBus[deviceId][channel]);
            }
         }
      }

      invokeFinalMixCallback(*devices, numDevices, numChannels, NumOutputSamples, mainBus);
   }

   // TODO: Apply compressor

   // TODO: Channel upmix/downmix, but I think we should let the audio driver (aka SDL) handle that

   if (type == AXDeviceType::TV) {
      // Copy the generated data out for later pickup
      memcpy(gTvSamples, mainBus, sizeof(Pcm16Sample) * numDevices * numChannels * NumOutputSamples);
   } else if (type == AXDeviceType::DRC) {
      // We currently just discard the generated DRC audio
   } else if (type == AXDeviceType::RMT) {
      // We also discard generated RMT audio
   } else {
      decaf_abort("Unexpected device type during copy-out");
   }
}

void
mixOutput(int32_t* buffer,
          uint16_t numSamples,
          uint16_t numChannels)
{
   // Decode audio samples from the source voices
   decodeVoiceSamples(numSamples);

   // Mix all the devices
   mixDevice(AXDeviceType::TV, numSamples);
   mixDevice(AXDeviceType::DRC, numSamples);
   mixDevice(AXDeviceType::RMT, numSamples);

   // Send off the TV device 0 data to be played on host
   for (auto i = 0; i < NumOutputSamples; ++i) {
      for (auto ch = 0; ch < numChannels; ++ch) {
         buffer[numChannels * i + ch] = fixed_to_data(gTvSamples[0][ch][i]);
      }
   }
}

} // namespace internal

AXResult
AXGetDeviceMode(AXDeviceType type,
                virt_ptr<AXDeviceMode> outMode)
{
   if (!outMode) {
      return AXResult::Success;
   }

   switch (type) {
   case AXDeviceType::TV:
      *outMode = sDeviceData->tvDevices.mode;
      break;
   case AXDeviceType::DRC:
      *outMode = sDeviceData->drcDevices.mode;
      break;
   case AXDeviceType::RMT:
      *outMode = sDeviceData->rmtDevices.mode;
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
   auto devices = internal::getDeviceGroup(type);

   switch (type) {
   case AXDeviceType::TV:
      devices->mode = mode;
      break;
   case AXDeviceType::DRC:
      devices->mode = mode;
      break;
   case AXDeviceType::RMT:
      devices->mode = mode;
      break;
   default:
      return AXResult::InvalidDeviceType;
   }

   return AXResult::Success;
}

AXResult
AXGetDeviceFinalMixCallback(AXDeviceType type,
                            virt_ptr<AXDeviceFinalMixCallback> outFunc)
{
   if (!outFunc) {
      return AXResult::Success;
   }

   auto devices = internal::getDeviceGroup(type);

   switch (type) {
   case AXDeviceType::TV:
      *outFunc = devices->finalMixCallback;
      break;
   case AXDeviceType::DRC:
      *outFunc = devices->finalMixCallback;
      break;
   case AXDeviceType::RMT:
      *outFunc = devices->finalMixCallback;
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
   auto devices = internal::getDeviceGroup(type);

   switch (type) {
   case AXDeviceType::TV:
      devices->finalMixCallback = func;
      break;
   case AXDeviceType::DRC:
      devices->finalMixCallback = func;
      break;
   case AXDeviceType::RMT:
      devices->finalMixCallback = func;
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
                 virt_ptr<AXAuxCallback> outCallback,
                 virt_ptr<virt_ptr<void>> outUserData)
{
   auto device = internal::getDevice(type, deviceId);

   if (!device) {
      return AXResult::InvalidDeviceType;
   }

   if (outCallback) {
      *outCallback = device->aux[auxId].callback;
   }

   if (outUserData) {
      *outUserData = device->aux[auxId].userData;
   }

   return AXResult::Success;
}

AXResult
AXRegisterAuxCallback(AXDeviceType type,
                      uint32_t deviceId,
                      AXAuxId auxId,
                      AXAuxCallback callback,
                      virt_ptr<void> userData)
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
      sDeviceData->tvDevices.compressor = !!compressor;
      break;
   case AXDeviceType::DRC:
      sDeviceData->drcDevices.compressor = !!compressor;
      break;
   case AXDeviceType::RMT:
      sDeviceData->rmtDevices.compressor = !!compressor;
      break;
   default:
      return AXResult::InvalidDeviceType;
   }

   return AXResult::Success;
}

AXResult
AXGetDeviceUpsampleStage(AXDeviceType type,
                         virt_ptr<BOOL> outUpsampleAfterFinalMixCallback)
{
   if (!outUpsampleAfterFinalMixCallback) {
      return AXResult::Success;
   }

   switch (type) {
   case AXDeviceType::TV:
      *outUpsampleAfterFinalMixCallback = sDeviceData->tvDevices.upsampleAfterFinalMix ? TRUE : FALSE;
      break;
   case AXDeviceType::DRC:
      *outUpsampleAfterFinalMixCallback = sDeviceData->drcDevices.upsampleAfterFinalMix ? TRUE : FALSE;
      break;
   case AXDeviceType::RMT:
      *outUpsampleAfterFinalMixCallback = sDeviceData->rmtDevices.upsampleAfterFinalMix ? TRUE : FALSE;
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
      sDeviceData->tvDevices.upsampleAfterFinalMix = !!upsampleAfterFinalMixCallback;
      break;
   case AXDeviceType::DRC:
      sDeviceData->drcDevices.upsampleAfterFinalMix = !!upsampleAfterFinalMixCallback;
      break;
   case AXDeviceType::RMT:
      sDeviceData->rmtDevices.upsampleAfterFinalMix = !!upsampleAfterFinalMixCallback;
      break;
   default:
      return AXResult::InvalidDeviceType;
   }

   return AXResult::Success;
}

AXResult
AXGetDeviceVolume(AXDeviceType type,
                  uint32_t deviceId,
                  virt_ptr<uint16_t> outVolume)
{
   auto device = internal::getDevice(type, deviceId);

   if (!device) {
      return AXResult::InvalidDeviceType;
   }

   if (outVolume) {
      *outVolume = fixed_to_data(device->volume);
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

   device->volume = fixed_from_data<ufixed_1_15_t>(volume);
   return AXResult::Success;
}

AXResult
AXGetAuxReturnVolume(AXDeviceType type,
                     uint32_t deviceId,
                     AXAuxId auxId,
                     virt_ptr<uint16_t> outVolume)
{
   auto device = internal::getDevice(type, deviceId);

   if (!device) {
      return AXResult::InvalidDeviceType;
   }

   if (outVolume) {
      *outVolume = fixed_to_data(device->aux[auxId].returnVolume);
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

   device->aux[auxId].returnVolume = fixed_from_data<ufixed_1_15_t>(volume);
   return AXResult::Success;
}

namespace internal
{

void
initDevices()
{
   for (auto &device : sDeviceData->tvDevices.devices) {
      device.volume = DefaultVolume;
      for (auto &aux : device.aux) {
         aux.returnVolume = DefaultVolume;
      }
   }
   for (auto &device : sDeviceData->drcDevices.devices) {
      device.volume = DefaultVolume;
      for (auto &aux : device.aux) {
         aux.returnVolume = DefaultVolume;
      }
   }
   for (auto &device : sDeviceData->rmtDevices.devices) {
      device.volume = DefaultVolume;
      for (auto &aux : device.aux) {
         aux.returnVolume = DefaultVolume;
      }
   }
}

} // namespace internal

void
Library::registerDeviceSymbols()
{
   RegisterFunctionExport(AXGetDeviceMode);
   RegisterFunctionExport(AXSetDeviceMode);
   RegisterFunctionExport(AXGetDeviceFinalMixCallback);
   RegisterFunctionExport(AXRegisterDeviceFinalMixCallback);
   RegisterFunctionExport(AXGetAuxCallback);
   RegisterFunctionExport(AXRegisterAuxCallback);
   RegisterFunctionExport(AXSetDeviceLinearUpsampler);
   RegisterFunctionExport(AXSetDeviceCompressor);
   RegisterFunctionExport(AXGetDeviceUpsampleStage);
   RegisterFunctionExport(AXSetDeviceUpsampleStage);
   RegisterFunctionExport(AXGetDeviceVolume);
   RegisterFunctionExport(AXSetDeviceVolume);
   RegisterFunctionExport(AXGetAuxReturnVolume);
   RegisterFunctionExport(AXSetAuxReturnVolume);

   RegisterDataInternal(sDeviceData);
}

} // namespace cafe::sndcore2
