#pragma once
#include "replay_parser.h"
#include "replay_ringbuffer.h"

#include <libcpu/be2_struct.h>
#include <libdecaf/decaf_pm4replay.h>
#include <libdecaf/src/cafe/cafe_tinyheap.h>
#include <libgpu/gpu_graphicsdriver.h>
#include <libgpu/latte/latte_pm4.h>

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>

class ReplayParserPM4 : public ReplayParser
{
   ReplayParserPM4() = default;
   ReplayParserPM4(const ReplayParserPM4 &) = delete;
   ReplayParserPM4(ReplayParserPM4 &&) = delete;

   ReplayParserPM4 &operator=(const ReplayParserPM4 &) = delete;
   ReplayParserPM4 &operator=(ReplayParserPM4 &&) = delete;

public:
   virtual ~ReplayParserPM4() = default;
   bool runUntilTimestamp(uint64_t timestamp) override;

   static std::unique_ptr<ReplayParser>
   Create(gpu::GraphicsDriver *driver,
          RingBuffer *ringBuffer,
          phys_ptr<cafe::TinyHeapPhysical> heap,
          const std::string &path);

private:
   bool handleCommandBuffer(void *buffer, uint32_t sizeBytes);
   void handleSetBuffer(decaf::pm4::CaptureSetBuffer &setBuffer);
   void handleRegisterSnapshot(phys_ptr<uint32_t> registers, uint32_t count);
   void handleMemoryLoad(decaf::pm4::CaptureMemoryLoad &load, std::vector<char> &data);
   bool scanType0(latte::pm4::HeaderType0 header, const gsl::span<be2_val<uint32_t>> &data);
   bool scanType3(latte::pm4::HeaderType3 header, const gsl::span<be2_val<uint32_t>> &data);
   bool scanCommandBuffer(void *words, uint32_t numWords);

private:
   gpu::GraphicsDriver *mGraphicsDriver = nullptr;
   RingBuffer *mRingBuffer = nullptr;
   std::ifstream mFile;

   phys_ptr<cafe::TinyHeapPhysical> mHeap = nullptr;
   phys_ptr<uint32_t> mRegisterStorage = nullptr;
};
