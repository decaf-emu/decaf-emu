#include "replay_parser_pm4.h"

#include <libdecaf/src/cafe/cafe_tinyheap.h>
#include <libgpu/latte/latte_pm4_commands.h>

using namespace latte::pm4;

std::unique_ptr<ReplayParser>
ReplayParserPM4::Create(gpu::GraphicsDriver *driver,
                        RingBuffer *ringBuffer,
                        phys_ptr<cafe::TinyHeapPhysical> heap,
                        const std::string &path)
{
   // Try open file
   std::ifstream file;
   file.open(path, std::ifstream::binary);
   if (!file.is_open()) {
      return {};
   }

   // Check magic header
   std::array<char, 4> magic;
   file.read(magic.data(), 4);
   if (magic != decaf::pm4::CaptureMagic) {
      return {};
   }

   // Allocate register storage
   auto allocPtr = phys_ptr<void> { nullptr };
   cafe::TinyHeap_Alloc(heap, 0x10000 * 4, 0x100, &allocPtr);
   if (!allocPtr) {
      return { };
   }

   auto self = new ReplayParserPM4 { };
   self->mGraphicsDriver = driver;
   self->mRingBuffer = ringBuffer;
   self->mHeap = heap;
   self->mRegisterStorage = phys_cast<uint32_t *>(allocPtr);
   self->mFile = std::move(file);

   return std::unique_ptr<ReplayParser> { self };
}

bool
ReplayParserPM4::runUntilTimestamp(uint64_t timestamp)
{
   std::vector<char> buffer;
   bool reachedTimestamp = false;
   mFile.clear();
   mFile.seekg(4, std::fstream::beg);

   while (true) {
      decaf::pm4::CapturePacket packet;
      mFile.read(reinterpret_cast<char *>(&packet), sizeof(decaf::pm4::CapturePacket));
      if (!mFile) {
         return false;
      }

      switch (packet.type) {
      case decaf::pm4::CapturePacket::CommandBuffer:
      {
         buffer.resize(packet.size);
         mFile.read(buffer.data(), buffer.size());

         if (!mFile) {
            return false;
         }

         handleCommandBuffer(buffer.data(), packet.size);
         break;
      }
      case decaf::pm4::CapturePacket::RegisterSnapshot:
      {
         decaf_check((packet.size % 4) == 0);
         auto numRegisters = packet.size / 4;
         mFile.read(reinterpret_cast<char *>(mRegisterStorage.getRawPointer()), packet.size);

         // Swap it into big endian, so we can write LOAD_ commands
         for (auto i = 0u; i < numRegisters; ++i) {
            mRegisterStorage[i] = byte_swap(mRegisterStorage[i]);
         }

         handleRegisterSnapshot(mRegisterStorage, numRegisters);
         mRingBuffer->waitTimestamp(mRingBuffer->flushCommandBuffer());
         break;
      }
      case decaf::pm4::CapturePacket::SetBuffer:
      {
         decaf::pm4::CaptureSetBuffer setBuffer;
         mFile.read(reinterpret_cast<char *>(&setBuffer), sizeof(decaf::pm4::CaptureSetBuffer));

         handleSetBuffer(setBuffer);
         mRingBuffer->waitTimestamp(mRingBuffer->flushCommandBuffer());
         break;
      }
      case decaf::pm4::CapturePacket::MemoryLoad:
      {
         decaf::pm4::CaptureMemoryLoad load;
         mFile.read(reinterpret_cast<char *>(&load), sizeof(decaf::pm4::CaptureMemoryLoad));

         if (!mFile) {
            return false;
         }

         buffer.resize(packet.size - sizeof(decaf::pm4::CaptureMemoryLoad));
         mFile.read(buffer.data(), buffer.size());

         if (!mFile) {
            return false;
         }

         handleMemoryLoad(load, buffer);
         break;
      }
      default:
         mFile.seekg(packet.size, std::ifstream::cur);
      }

      if (packet.timestamp >= timestamp) {
         reachedTimestamp = true;
         break;
      }
   }

   // Flush ringbuffer to gpu
   mRingBuffer->waitTimestamp(mRingBuffer->flushCommandBuffer());
   return reachedTimestamp;
}

bool
ReplayParserPM4::handleCommandBuffer(void *buffer, uint32_t sizeBytes)
{
   auto numWords = sizeBytes / 4;
   mRingBuffer->writeBuffer(buffer, numWords);
   return scanCommandBuffer(buffer, numWords);
}

void
ReplayParserPM4::handleSetBuffer(decaf::pm4::CaptureSetBuffer &setBuffer)
{
   auto isTv = (setBuffer.type == decaf::pm4::CaptureSetBuffer::TvBuffer) ? 1u : 0u;

   mRingBuffer->writePM4(DecafSetBuffer {
      isTv ? latte::pm4::ScanTarget::TV : latte::pm4::ScanTarget::DRC,
      setBuffer.address,
      setBuffer.bufferingMode,
      setBuffer.width,
      setBuffer.height
   });
}

void
ReplayParserPM4::handleRegisterSnapshot(phys_ptr<uint32_t> registers, uint32_t count)
{
   // Enable loading of registers
   auto LOAD_CONTROL = latte::CONTEXT_CONTROL_ENABLE::get(0)
      .ENABLE_CONFIG_REG(true)
      .ENABLE_CONTEXT_REG(true)
      .ENABLE_ALU_CONST(true)
      .ENABLE_BOOL_CONST(true)
      .ENABLE_LOOP_CONST(true)
      .ENABLE_RESOURCE(true)
      .ENABLE_SAMPLER(true)
      .ENABLE_CTL_CONST(true)
      .ENABLE_ORDINAL(true);

   auto SHADOW_ENABLE = latte::CONTEXT_CONTROL_ENABLE::get(0);

   mRingBuffer->writePM4(ContextControl {
      LOAD_CONTROL,
      SHADOW_ENABLE
   });

   // Write all the register load packets!
   static std::pair<uint32_t, uint32_t>
   LoadConfigRange[] = { { 0, (latte::Register::ConfigRegisterEnd - latte::Register::ConfigRegisterBase) / 4 }, };

   mRingBuffer->writePM4(LoadConfigReg {
      phys_cast<phys_addr>(registers + (latte::Register::ConfigRegisterBase / 4)),
      gsl::make_span(LoadConfigRange)
   });

   static std::pair<uint32_t, uint32_t>
   LoadContextRange[] = { { 0, (latte::Register::ContextRegisterEnd - latte::Register::ContextRegisterBase) / 4 }, };

   mRingBuffer->writePM4(LoadContextReg {
      phys_cast<phys_addr>(registers + (latte::Register::ContextRegisterBase / 4)),
      gsl::make_span(LoadContextRange)
   });

   static std::pair<uint32_t, uint32_t>
   LoadAluConstRange[] = { { 0, (latte::Register::AluConstRegisterEnd - latte::Register::AluConstRegisterBase) / 4 }, };

   mRingBuffer->writePM4(LoadAluConst {
      phys_cast<phys_addr>(registers + (latte::Register::AluConstRegisterBase / 4)),
      gsl::make_span(LoadAluConstRange)
   });

   static std::pair<uint32_t, uint32_t>
   LoadResourceRange[] = { { 0, (latte::Register::ResourceRegisterEnd - latte::Register::ResourceRegisterBase) / 4 }, };

   mRingBuffer->writePM4(latte::pm4::LoadResource {
      phys_cast<phys_addr>(registers + (latte::Register::ResourceRegisterBase / 4)),
      gsl::make_span(LoadResourceRange)
   });

   static std::pair<uint32_t, uint32_t>
   LoadSamplerRange[] = { { 0, (latte::Register::SamplerRegisterEnd - latte::Register::SamplerRegisterBase) / 4 }, };

   mRingBuffer->writePM4(LoadSampler {
      phys_cast<phys_addr>(registers + (latte::Register::SamplerRegisterBase / 4)),
      gsl::make_span(LoadSamplerRange)
   });

   static std::pair<uint32_t, uint32_t>
   LoadControlRange[] = { { 0, (latte::Register::ControlRegisterEnd - latte::Register::ControlRegisterBase) / 4 }, };

   mRingBuffer->writePM4(LoadControlConst {
      phys_cast<phys_addr>(registers + (latte::Register::ControlRegisterBase / 4)),
      gsl::make_span(LoadControlRange)
   });

   static std::pair<uint32_t, uint32_t>
   LoadLoopRange[] = { { 0, (latte::Register::LoopConstRegisterEnd - latte::Register::LoopConstRegisterBase) / 4 }, };

   mRingBuffer->writePM4(LoadLoopConst {
      phys_cast<phys_addr>(registers + (latte::Register::LoopConstRegisterBase / 4)),
      gsl::make_span(LoadLoopRange)
   });

   static std::pair<uint32_t, uint32_t>
   LoadBoolRange[] = { { 0, (latte::Register::BoolConstRegisterEnd - latte::Register::BoolConstRegisterBase) / 4 }, };

   mRingBuffer->writePM4(LoadLoopConst {
      phys_cast<phys_addr>(registers + (latte::Register::BoolConstRegisterBase / 4)),
      gsl::make_span(LoadBoolRange)
   });
}

void
ReplayParserPM4::handleMemoryLoad(decaf::pm4::CaptureMemoryLoad &load,
                                  std::vector<char> &data)
{
   std::memcpy(phys_cast<void *>(load.address).getRawPointer(),
               data.data(), data.size());

   mGraphicsDriver->notifyCpuFlush(load.address,
                                    static_cast<uint32_t>(data.size()));
}

bool
ReplayParserPM4::scanType0(HeaderType0 header,
                           const gsl::span<be2_val<uint32_t>> &data)
{
   return false;
}

bool
ReplayParserPM4::scanType3(HeaderType3 header,
                           const gsl::span<be2_val<uint32_t>> &data)
{
   if (header.opcode() == IT_OPCODE::DECAF_SWAP_BUFFERS) {
      return true;
   }

   if (header.opcode() == IT_OPCODE::INDIRECT_BUFFER ||
         header.opcode() == IT_OPCODE::INDIRECT_BUFFER_PRIV) {
      return scanCommandBuffer(phys_cast<void *>(phys_addr { data[0].value() }).getRawPointer(),
                                 data[2]);
   }

   return false;
}

bool
ReplayParserPM4::scanCommandBuffer(void *words, uint32_t numWords)
{
   auto buffer = reinterpret_cast<be2_val<uint32_t> *>(words);
   auto foundSwap = false;

   for (auto pos = size_t { 0u }; pos < numWords; ) {
      auto header = Header::get(buffer[pos]);
      auto size = size_t { 0u };

      switch (header.type()) {
      case PacketType::Type0:
      {
         auto header0 = HeaderType0::get(header.value);
         size = header0.count() + 1;

         decaf_check(pos + size < numWords);
         foundSwap |= scanType0(header0, gsl::make_span(buffer + pos + 1, size));
         break;
      }
      case PacketType::Type3:
      {
         auto header3 = HeaderType3::get(header.value);
         size = header3.size() + 1;

         decaf_check(pos + size < numWords);
         foundSwap |= scanType3(header3, gsl::make_span(buffer + pos + 1, size));
         break;
      }
      case PacketType::Type2:
      {
         // This is a filler packet, like a "nop", ignore it
         break;
      }
      case PacketType::Type1:
      default:
         size = numWords;
         break;
      }

      pos += size + 1;
   }

   return foundSwap;
}
