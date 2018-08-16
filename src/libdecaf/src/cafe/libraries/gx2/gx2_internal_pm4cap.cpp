#include "decaf_pm4replay.h"
#include "gx2_display.h"
#include "gx2_event.h"
#include "gx2_internal_pm4cap.h"

#include <array>
#include <addrlib/addrinterface.h>
#include <common/byte_swap.h>
#include <common/log.h>
#include <common/platform_dir.h>
#include <common/murmur3.h>
#include <fmt/format.h>
#include <fstream>
#include <gsl.h>
#include <libgpu/gpu_tiling.h>
#include <libgpu/latte/latte_constants.h>
#include <libgpu/latte/latte_formats.h>
#include <libgpu/latte/latte_pm4.h>
#include <libgpu/latte/latte_pm4_commands.h>
#include <libgpu/latte/latte_pm4_reader.h>
#include <vector>

using decaf::pm4::CaptureMagic;
using decaf::pm4::CapturePacket;
using decaf::pm4::CaptureMemoryLoad;
using decaf::pm4::CaptureSetBuffer;
using namespace latte::pm4;

static const auto
HashAllMemory = true;

static const auto
HashShadowState = true;

namespace cafe::gx2::internal
{

class Recorder
{
   struct RecordedMemory
   {
      virt_addr start;
      virt_addr end;
      uint64_t hash[2];
   };

public:
   Recorder()
   {
      mRegisters.fill(0);
   }

   bool
   requestStart(const std::string &path)
   {
      decaf_check(mState == CaptureState::Disabled);
      std::unique_lock<std::mutex> lock { mMutex };
      mOut.open(path, std::fstream::binary);

      if (!mOut.is_open()) {
         return false;
      }

      // Write magic header
      mOut.write(CaptureMagic.data(), CaptureMagic.size());

      // Set intial state
      mRecordedMemory.clear();
      mState = CaptureState::WaitStartNextFrame;

      return true;
   }

   void
   requestStop()
   {
      decaf_check(mState == CaptureState::Enabled);
      std::unique_lock<std::mutex> lock { mMutex };
      mState = CaptureState::WaitEndNextFrame;
   }

   void
   setCaptureNumFrames(size_t count)
   {
      mCaptureNumFrames = count;
   }

   CaptureState
   state() const
   {
      return mState;
   }

   void
   swap()
   {
      std::unique_lock<std::mutex> lock { mMutex };

      if (mState == CaptureState::WaitStartNextFrame) {
         start();
         mCapturedFrames = 0;
      } else if (mState == CaptureState::WaitEndNextFrame) {
         stop();
         mCaptureNumFrames = 0;
      }

      ++mCapturedFrames;

      if (mCapturedFrames == mCaptureNumFrames) {
         mState = CaptureState::WaitEndNextFrame;
      }
   }

   void
   commandBuffer(CommandBuffer *buffer)
   {
      decaf_check(mState == CaptureState::Enabled || mState == CaptureState::WaitEndNextFrame);
      std::unique_lock<std::mutex> lock { mMutex };
      auto size = buffer->curSize * 4;
      scanCommandBuffer(buffer->buffer, buffer->curSize);

      CapturePacket packet;
      packet.type = CapturePacket::CommandBuffer;
      packet.size = size;
      writePacket(packet);
      writeData(buffer->buffer, packet.size);
   }

   void
   cpuFlush(void *buffer,
            uint32_t size)
   {
      decaf_check(mState == CaptureState::Enabled || mState == CaptureState::WaitEndNextFrame);
      std::unique_lock<std::mutex> lock { mMutex };
      auto addr = cpu::translate(buffer);
      trackMemory(CaptureMemoryLoad::CpuFlush, addr, size);
   }

   void
   gpuFlush(void *buffer,
            uint32_t size)
   {
      decaf_check(mState == CaptureState::Enabled || mState == CaptureState::WaitEndNextFrame);
      // Not sure if we need to do something here...
   }

   void
   syncRegisters(const uint32_t *registers,
                 uint32_t size)
   {
      decaf_check(mState == CaptureState::WaitStartNextFrame);
      decaf_check(size == mRegisters.size());
      std::memcpy(mRegisters.data(), registers, size * sizeof(uint32_t));
   }

private:
   void
   start()
   {
      decaf_check(mState == CaptureState::Disabled || mState == CaptureState::WaitStartNextFrame);
      mState = CaptureState::Enabled;
      writeRegisterSnapshot();
      writeDisplayInfo();
   }

   void
   stop()
   {
      decaf_check(mState == CaptureState::Enabled || mState == CaptureState::WaitEndNextFrame);
      mOut.close();
      mState = CaptureState::Disabled;
   }

   void
   writeRegisterSnapshot()
   {
      CapturePacket packet;
      packet.type = CapturePacket::RegisterSnapshot;
      packet.size = static_cast<uint32_t>(mRegisters.size() * sizeof(uint32_t));
      writePacket(packet);
      writeData(mRegisters.data(), packet.size);
   }

   void
   writeDisplayInfo()
   {
      auto tvInfo = getTvBufferInfo();
      if (tvInfo->buffer) {
         CapturePacket packet;
         packet.type = CapturePacket::SetBuffer;
         packet.size = sizeof(CaptureSetBuffer);
         writePacket(packet);

         CaptureSetBuffer setBuffer;
         setBuffer.type = CaptureSetBuffer::TvBuffer;
         setBuffer.address = static_cast<uint32_t>(virt_cast<virt_addr>(tvInfo->buffer));
         setBuffer.size = tvInfo->size;
         setBuffer.renderMode = tvInfo->tvRenderMode;
         setBuffer.surfaceFormat = tvInfo->surfaceFormat;
         setBuffer.bufferingMode = tvInfo->bufferingMode;
         setBuffer.width = tvInfo->width;
         setBuffer.height = tvInfo->height;
         writeData(&setBuffer, packet.size);
      }

      auto drcInfo = getDrcBufferInfo();
      if (drcInfo->buffer) {
         CapturePacket packet;
         packet.type = CapturePacket::SetBuffer;
         packet.size = sizeof(CaptureSetBuffer);
         writePacket(packet);

         CaptureSetBuffer setBuffer;
         setBuffer.type = CaptureSetBuffer::DrcBuffer;
         setBuffer.address = static_cast<uint32_t>(virt_cast<virt_addr>(drcInfo->buffer));
         setBuffer.size = drcInfo->size;
         setBuffer.renderMode = drcInfo->drcRenderMode;
         setBuffer.surfaceFormat = drcInfo->surfaceFormat;
         setBuffer.bufferingMode = drcInfo->bufferingMode;
         setBuffer.width = drcInfo->width;
         setBuffer.height = drcInfo->height;
         writeData(&setBuffer, packet.size);
      }
   }

   void
   writePacket(CapturePacket &packet)
   {
      mOut.write(reinterpret_cast<const char *>(&packet), sizeof(CapturePacket));
   }

   void
   writeData(void *data, uint32_t size)
   {
      mOut.write(reinterpret_cast<const char *>(data), size);
   }

   void
   writeMemoryLoad(CaptureMemoryLoad::MemoryType type,
                   void *buffer,
                   uint32_t size)
   {
      CaptureMemoryLoad load;
      load.type = type;
      load.address = mem::untranslate(buffer);

      CapturePacket packet;
      packet.type = CapturePacket::MemoryLoad;
      packet.size = size + sizeof(CaptureMemoryLoad);
      writePacket(packet);
      writeData(&load, sizeof(CaptureMemoryLoad));
      writeData(buffer, size);
   }

   void
   scanCommandBuffer(uint32_t *words, uint32_t numWords)
   {
      std::vector<uint32_t> swapped;
      swapped.resize(numWords);

      for (auto i = 0u; i < swapped.size(); ++i) {
         swapped[i] = byte_swap(words[i]);
      }

      auto buffer = swapped.data();
      auto bufferSize = swapped.size();

      for (auto pos = size_t { 0u }; pos < bufferSize; ) {
         auto header = Header::get(buffer[pos]);
         auto size = size_t { 0u };

         switch (header.type()) {
         case PacketType::Type0:
         {
            auto header0 = HeaderType0::get(header.value);
            size = header0.count() + 1;

            decaf_check(pos + size < bufferSize);
            scanType0(header0, gsl::make_span(&buffer[pos + 1], size));
            break;
         }
         case PacketType::Type3:
         {
            auto header3 = HeaderType3::get(header.value);
            size = header3.size() + 1;

            decaf_check(pos + size < bufferSize);
            scanType3(header3, gsl::make_span(&buffer[pos + 1], size));
            break;
         }
         case PacketType::Type2:
         {
            // This is a filler packet, like a "nop", ignore it
            break;
         }
         case PacketType::Type1:
         default:
            gLog->error("Invalid packet header type {}, header = 0x{:08X}", header.type(), header.value);
            size = bufferSize;
            break;
         }

         pos += size + 1;
      }
   }

   void
   scanType0(HeaderType0 header,
             const gsl::span<uint32_t> &data)
   {
   }

   ADDR_COMPUTE_SURFACE_INFO_OUTPUT
   getSurfaceInfo(uint32_t pitch,
                  uint32_t height,
                  uint32_t depth,
                  uint32_t aa,
                  uint32_t level,
                  bool isScanBuffer,
                  bool isDepthBuffer,
                  latte::SQ_TEX_DIM dim,
                  latte::SQ_DATA_FORMAT format,
                  latte::SQ_TILE_MODE tileMode)
   {
      ADDR_COMPUTE_SURFACE_INFO_OUTPUT output;
      auto hwFormat = format;
      auto width = std::max<uint32_t>(1u, pitch >> level);
      auto numSlices = 1u;

      switch (dim) {
      case latte::SQ_TEX_DIM::DIM_1D:
         height = 1;
         numSlices = 1;
         break;
      case latte::SQ_TEX_DIM::DIM_2D:
         height = std::max<uint32_t>(1u, height >> level);
         numSlices = 1;
         break;
      case latte::SQ_TEX_DIM::DIM_3D:
         height = std::max<uint32_t>(1u, height >> level);
         numSlices = std::max<uint32_t>(1u, depth >> level);
         break;
      case latte::SQ_TEX_DIM::DIM_CUBEMAP:
         height = std::max<uint32_t>(1u, height >> level);
         numSlices = std::max<uint32_t>(6u, depth);
         break;
      case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
         height = 1;
         numSlices = depth;
         break;
      case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
         height = std::max<uint32_t>(1u, height >> level);
         numSlices = depth;
         break;
      case latte::SQ_TEX_DIM::DIM_2D_MSAA:
         height = std::max<uint32_t>(1u, height >> level);
         numSlices = 1;
         break;
      case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
         height = std::max<uint32_t>(1u, height >> level);
         numSlices = depth;
         break;
      }

      std::memset(&output, 0, sizeof(ADDR_COMPUTE_SURFACE_INFO_OUTPUT));
      output.size = sizeof(ADDR_COMPUTE_SURFACE_INFO_OUTPUT);

      ADDR_COMPUTE_SURFACE_INFO_INPUT input;
      memset(&input, 0, sizeof(ADDR_COMPUTE_SURFACE_INFO_INPUT));
      input.size = sizeof(ADDR_COMPUTE_SURFACE_INFO_INPUT);
      input.tileMode = static_cast<AddrTileMode>(tileMode & 0xF);
      input.format = static_cast<AddrFormat>(hwFormat);
      input.bpp = latte::getDataFormatBitsPerElement(format);
      input.width = width;
      input.height = height;
      input.numSlices = numSlices;

      input.numSamples = 1 << aa;
      input.numFrags = input.numSamples;

      input.slice = 0;
      input.mipLevel = level;

      if (dim == latte::SQ_TEX_DIM::DIM_CUBEMAP) {
         input.flags.cube = 1;
      }

      if (dim == latte::SQ_TEX_DIM::DIM_3D) {
         input.flags.volume = 1;
      }

      if (isDepthBuffer) {
         input.flags.depth = 1;
      }

      if (isScanBuffer) {
         input.flags.display = 1;
      }

      input.flags.inputBaseMap = (level == 0);
      AddrComputeSurfaceInfo(gpu::getAddrLibHandle(), &input, &output);
      return output;
   }

   void
   trackSurface(virt_addr baseAddress,
                uint32_t pitch,
                uint32_t height,
                uint32_t depth,
                uint32_t aa,
                uint32_t level,
                bool isDepthBuffer,
                bool isScanBuffer,
                latte::SQ_TEX_DIM dim,
                latte::SQ_DATA_FORMAT format,
                latte::SQ_TILE_MODE tileMode)
   {
      if (!baseAddress || !pitch || !height) {
         return;
      }

      // Adjust address for swizzling
      if (tileMode >= latte::SQ_TILE_MODE::TILED_2D_THIN1) {
         baseAddress &= ~(0x800 - 1);
      } else {
         baseAddress &= ~(0x100 - 1);
      }

      // Calculate size
      auto info = getSurfaceInfo(pitch, height, depth, aa, level, isDepthBuffer, isScanBuffer, dim, format, tileMode);

      // TODO: Use align? info.baseAlign;

      // Track that badboy
      trackMemory(CaptureMemoryLoad::Surface, baseAddress, static_cast<uint32_t>(info.surfSize));
   }

   void
   trackColorBuffer(latte::CB_COLORN_BASE cb_color_base,
                    latte::CB_COLORN_SIZE cb_color_size,
                    latte::CB_COLORN_INFO cb_color_info)
   {
      auto pitch_tile_max = cb_color_size.PITCH_TILE_MAX();
      auto slice_tile_max = cb_color_size.SLICE_TILE_MAX();
      auto pitch = static_cast<uint32_t>((pitch_tile_max + 1) * latte::MicroTileWidth);
      auto height = static_cast<uint32_t>(((slice_tile_max + 1) * (latte::MicroTileWidth * latte::MicroTileHeight)) / pitch);
      auto addr = (cb_color_base.BASE_256B() << 8);

      if (!addr || !pitch || !height) {
         return;
      }

      auto format = static_cast<latte::SQ_DATA_FORMAT>(cb_color_info.FORMAT());
      auto tileMode = latte::getArrayModeTileMode(cb_color_info.ARRAY_MODE());

      // Disabled for now, because it's a pointless upload
      // trackSurface(addr, pitch, height, 1, latte::SQ_TEX_DIM::DIM_2D, format, tileMode);
   }

   void
   trackDepthBuffer(latte::DB_DEPTH_BASE db_depth_base,
                    latte::DB_DEPTH_SIZE db_depth_size,
                    latte::DB_DEPTH_INFO db_depth_info)
   {
      auto pitch_tile_max = db_depth_size.PITCH_TILE_MAX();
      auto slice_tile_max = db_depth_size.SLICE_TILE_MAX();
      auto pitch = static_cast<uint32_t>((pitch_tile_max + 1) * latte::MicroTileWidth);
      auto height = static_cast<uint32_t>(((slice_tile_max + 1) * (latte::MicroTileWidth * latte::MicroTileHeight)) / pitch);
      auto addr = (db_depth_base.BASE_256B() << 8);

      if (!addr || !pitch || !height) {
         return;
      }

      auto format = static_cast<latte::SQ_DATA_FORMAT>(db_depth_info.FORMAT());
      auto tileMode = latte::getArrayModeTileMode(db_depth_info.ARRAY_MODE());

      // Disabled for now, because it's a pointless upload
      //trackSurface(addr, pitch, height, 1, latte::SQ_TEX_DIM::DIM_2D, format, tileMode);
   }

   void
      trackActiveShaders()
   {
      auto pgm_start_fs = getRegister<latte::SQ_PGM_START_FS>(latte::Register::SQ_PGM_START_FS);
      auto pgm_size_fs = getRegister<latte::SQ_PGM_SIZE_FS>(latte::Register::SQ_PGM_SIZE_FS);
      trackMemory(CaptureMemoryLoad::FetchShader,
                  virt_addr { pgm_start_fs.PGM_START() << 8 },
                  pgm_size_fs.PGM_SIZE() << 3);

      auto pgm_start_vs = getRegister<latte::SQ_PGM_START_VS>(latte::Register::SQ_PGM_START_VS);
      auto pgm_size_vs = getRegister<latte::SQ_PGM_SIZE_VS>(latte::Register::SQ_PGM_SIZE_VS);
      trackMemory(CaptureMemoryLoad::VertexShader,
                  virt_addr { pgm_start_vs.PGM_START() << 8 },
                  pgm_size_vs.PGM_SIZE() << 3);

      auto pgm_start_ps = getRegister<latte::SQ_PGM_START_PS>(latte::Register::SQ_PGM_START_PS);
      auto pgm_size_ps = getRegister<latte::SQ_PGM_SIZE_PS>(latte::Register::SQ_PGM_SIZE_PS);
      trackMemory(CaptureMemoryLoad::PixelShader,
                  virt_addr { pgm_start_ps.PGM_START() << 8 },
                  pgm_size_ps.PGM_SIZE() << 3);

      auto pgm_start_gs = getRegister<latte::SQ_PGM_START_GS>(latte::Register::SQ_PGM_START_GS);
      auto pgm_size_gs = getRegister<latte::SQ_PGM_SIZE_GS>(latte::Register::SQ_PGM_SIZE_GS);
      trackMemory(CaptureMemoryLoad::GeometryShader,
                  virt_addr { pgm_start_gs.PGM_START() << 8 },
                  pgm_size_gs.PGM_SIZE() << 3);
   }

   void
   trackActiveAttribBuffers()
   {
      for (auto i = 0u; i < latte::MaxAttributes; ++i) {
         auto resourceOffset = (latte::SQ_RES_OFFSET::VS_ATTRIB_RESOURCE_0 + i) * 7;
         auto sq_vtx_constant_word0 = getRegister<latte::SQ_VTX_CONSTANT_WORD0_N>(latte::Register::SQ_RESOURCE_WORD0_0 + 4 * resourceOffset);
         auto sq_vtx_constant_word1 = getRegister<latte::SQ_VTX_CONSTANT_WORD1_N>(latte::Register::SQ_RESOURCE_WORD1_0 + 4 * resourceOffset);

         trackMemory(CaptureMemoryLoad::AttributeBuffer,
                     virt_addr { sq_vtx_constant_word0.BASE_ADDRESS() },
                     sq_vtx_constant_word1.SIZE() + 1);
      }
   }

   void
   trackActiveUniforms()
   {
      for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
         auto sq_alu_const_cache_vs = getRegister<uint32_t>(latte::Register::SQ_ALU_CONST_CACHE_VS_0 + 4 * i);
         auto sq_alu_const_buffer_size_vs = getRegister<uint32_t>(latte::Register::SQ_ALU_CONST_BUFFER_SIZE_VS_0 + 4 * i);

         trackMemory(CaptureMemoryLoad::UniformBuffer,
                     virt_addr { sq_alu_const_cache_vs << 8 },
                     sq_alu_const_buffer_size_vs << 8);
      }
   }

   void
   trackActiveTextures()
   {
      for (auto i = 0u; i < latte::MaxTextures; ++i) {
         auto resourceOffset = (latte::SQ_RES_OFFSET::PS_TEX_RESOURCE_0 + i) * 7;
         auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_RESOURCE_WORD0_0 + 4 * resourceOffset);
         auto sq_tex_resource_word1 = getRegister<latte::SQ_TEX_RESOURCE_WORD1_N>(latte::Register::SQ_RESOURCE_WORD1_0 + 4 * resourceOffset);
         auto sq_tex_resource_word2 = getRegister<latte::SQ_TEX_RESOURCE_WORD2_N>(latte::Register::SQ_RESOURCE_WORD2_0 + 4 * resourceOffset);
         auto sq_tex_resource_word3 = getRegister<latte::SQ_TEX_RESOURCE_WORD3_N>(latte::Register::SQ_RESOURCE_WORD3_0 + 4 * resourceOffset);
         auto sq_tex_resource_word4 = getRegister<latte::SQ_TEX_RESOURCE_WORD4_N>(latte::Register::SQ_RESOURCE_WORD4_0 + 4 * resourceOffset);
         auto sq_tex_resource_word5 = getRegister<latte::SQ_TEX_RESOURCE_WORD5_N>(latte::Register::SQ_RESOURCE_WORD5_0 + 4 * resourceOffset);
         auto sq_tex_resource_word6 = getRegister<latte::SQ_TEX_RESOURCE_WORD6_N>(latte::Register::SQ_RESOURCE_WORD6_0 + 4 * resourceOffset);

         trackSurface(virt_addr { sq_tex_resource_word2.BASE_ADDRESS() << 8 },
                      (sq_tex_resource_word0.PITCH() + 1) * 8,
                      sq_tex_resource_word1.TEX_HEIGHT() + 1,
                      sq_tex_resource_word1.TEX_DEPTH() + 1,
                      sq_tex_resource_word5.LAST_LEVEL(),
                      0,
                      sq_tex_resource_word0.TILE_TYPE() == 1,
                      false,
                      sq_tex_resource_word0.DIM(),
                      sq_tex_resource_word1.DATA_FORMAT(),
                      sq_tex_resource_word0.TILE_MODE());
      }
   }

   void
   trackActiveColorBuffer()
   {
      for (auto i = 0u; i < latte::MaxRenderTargets; ++i) {
         auto cb_color_base = getRegister<latte::CB_COLORN_BASE>(latte::Register::CB_COLOR0_BASE + i * 4);
         auto cb_color_size = getRegister<latte::CB_COLORN_SIZE>(latte::Register::CB_COLOR0_SIZE + i * 4);
         auto cb_color_info = getRegister<latte::CB_COLORN_INFO>(latte::Register::CB_COLOR0_INFO + i * 4);

         trackColorBuffer(cb_color_base, cb_color_size, cb_color_info);
      }
   }

   void
   trackActiveDepthBuffer()
   {
      auto db_depth_base = getRegister<latte::DB_DEPTH_BASE>(latte::Register::DB_DEPTH_BASE);
      auto db_depth_size = getRegister<latte::DB_DEPTH_SIZE>(latte::Register::DB_DEPTH_SIZE);
      auto db_depth_info = getRegister<latte::DB_DEPTH_INFO>(latte::Register::DB_DEPTH_INFO);

      trackDepthBuffer(db_depth_base, db_depth_size, db_depth_info);
   }

   void
   trackReadyDraw()
   {
      trackActiveShaders();
      trackActiveAttribBuffers();
      trackActiveUniforms();
      trackActiveTextures();
      trackActiveColorBuffer();
      trackActiveDepthBuffer();
   }

   void
   scanSetRegister(latte::Register reg,
                   uint32_t value)
   {
      mRegisters[reg / 4] = value;
   }

   void
   scanLoadRegisters(latte::Register base,
                     virt_ptr<uint32_t> src,
                     const gsl::span<std::pair<uint32_t, uint32_t>> &registers)
   {
      for (auto &range : registers) {
         auto start = range.first;
         auto count = range.second;

         for (auto j = start; j < start + count; ++j) {
            scanSetRegister(static_cast<latte::Register>(base + j * 4), src[j]);
         }
      }
   }

   template<typename Type>
   Type
   getRegister(uint32_t id)
   {
      return bit_cast<Type>(mRegisters[id / 4]);
   }

   void
   scanType3(HeaderType3 header,
             const gsl::span<uint32_t> &rawData)
   {
      PacketReader reader { rawData };

      switch (header.opcode()) {
      case IT_OPCODE::DECAF_COPY_COLOR_TO_SCAN:
      {
         auto data = read<DecafCopyColorToScan>(reader);
         trackColorBuffer(data.cb_color_base, data.cb_color_size, data.cb_color_info);
         break;
      }
      case IT_OPCODE::DECAF_SWAP_BUFFERS:
         // Nothing to track!
         break;
      case IT_OPCODE::DECAF_CLEAR_COLOR:
      {
         auto data = read<DecafClearColor>(reader);
         trackColorBuffer(data.cb_color_base, data.cb_color_size, data.cb_color_info);
         break;
      }
      case IT_OPCODE::DECAF_CLEAR_DEPTH_STENCIL:
      {
         auto data = read<DecafClearDepthStencil>(reader);
         trackDepthBuffer(data.db_depth_base, data.db_depth_size, data.db_depth_info);
         break;
      }
      case IT_OPCODE::DECAF_SET_BUFFER:
         // Nothing to track!
         break;
      case IT_OPCODE::DECAF_DEBUGMARKER:
         // Nothing to track!
         break;
      case IT_OPCODE::DECAF_OSSCREEN_FLIP:
         decaf_abort("pm4 capture not enabled for OSScreen api");
         break;
      case IT_OPCODE::DECAF_COPY_SURFACE:
      {
         auto data = read<DecafCopySurface>(reader);
         trackSurface(virt_addr { data.srcImage }, data.srcPitch, data.srcHeight, data.srcDepth, 0, data.srcLevel, false, false, data.srcDim, data.srcFormat, data.srcTileMode);
         break;
      }
      case IT_OPCODE::DECAF_SET_SWAP_INTERVAL:
         // Nothing to track!
         break;
      case IT_OPCODE::DRAW_INDEX_AUTO:
         trackReadyDraw();
         break;
      case IT_OPCODE::DRAW_INDEX_2:
      {
         auto data = read<DrawIndex2>(reader);
         auto vgt_dma_index_type = getRegister<latte::VGT_DMA_INDEX_TYPE>(latte::Register::VGT_DMA_INDEX_TYPE);
         auto indexByteSize = 4u;

         if (vgt_dma_index_type.INDEX_TYPE() == latte::VGT_INDEX_TYPE::INDEX_16) {
            indexByteSize = 2u;
         }

         trackMemory(CaptureMemoryLoad::IndexBuffer, cpu::translate(data.addr), data.count * indexByteSize);
         trackReadyDraw();
         break;
      }
      case IT_OPCODE::DRAW_INDEX_IMMD:
         trackReadyDraw();
         break;
      case IT_OPCODE::INDEX_TYPE:
      {
         auto data = read<IndexType>(reader);
         mRegisters[latte::Register::VGT_DMA_INDEX_TYPE / 4] = data.type.value;
         break;
      }
      case IT_OPCODE::NUM_INSTANCES:
      {
         auto data = read<NumInstances>(reader);
         mRegisters[latte::Register::VGT_DMA_NUM_INSTANCES / 4] = data.count;
         break;
      }
      case IT_OPCODE::SET_ALU_CONST:
      {
         auto data = read<SetAluConsts>(reader);

         for (auto i = 0u; i < data.values.size(); ++i) {
            scanSetRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
         }

         break;
      }
      case IT_OPCODE::SET_CONFIG_REG:
      {
         auto data = read<SetConfigRegs>(reader);

         for (auto i = 0u; i < data.values.size(); ++i) {
            scanSetRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
         }

         break;
      }
      case IT_OPCODE::SET_CONTEXT_REG:
      {
         auto data = read<SetContextRegs>(reader);

         for (auto i = 0u; i < data.values.size(); ++i) {
            scanSetRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
         }

         break;
      }
      case IT_OPCODE::SET_CTL_CONST:
      {
         auto data = read<SetControlConstants>(reader);

         for (auto i = 0u; i < data.values.size(); ++i) {
            scanSetRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
         }

         break;
      }
      case IT_OPCODE::SET_LOOP_CONST:
      {
         auto data = read<SetLoopConsts>(reader);

         for (auto i = 0u; i < data.values.size(); ++i) {
            scanSetRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
         }

         break;
      }
      case IT_OPCODE::SET_SAMPLER:
      {
         auto data = read<SetSamplers>(reader);

         for (auto i = 0u; i < data.values.size(); ++i) {
            scanSetRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
         }

         break;
      }
      case IT_OPCODE::SET_RESOURCE:
      {
         auto data = read<SetResources>(reader);
         auto id = latte::Register::ResourceRegisterBase + (4 * data.id);

         for (auto i = 0u; i < data.values.size(); ++i) {
            scanSetRegister(static_cast<latte::Register>(id + i * 4), data.values[i]);
         }

         break;
      }
      case IT_OPCODE::LOAD_CONFIG_REG:
      {
         auto data = read<LoadControlConst>(reader);
         scanLoadRegisters(latte::Register::ConfigRegisterBase, data.addr, data.values);
         trackMemory(CaptureMemoryLoad::ShadowState, virt_cast<virt_addr>(data.addr), 0xB00 * 4);
         break;
      }
      case IT_OPCODE::LOAD_CONTEXT_REG:
      {
         auto data = read<LoadControlConst>(reader);
         scanLoadRegisters(latte::Register::ContextRegisterBase, data.addr, data.values);
         trackMemory(CaptureMemoryLoad::ShadowState, virt_cast<virt_addr>(data.addr), 0x400 * 4);
         break;
      }
      case IT_OPCODE::LOAD_ALU_CONST:
      {
         auto data = read<LoadControlConst>(reader);
         scanLoadRegisters(latte::Register::AluConstRegisterBase, data.addr, data.values);
         trackMemory(CaptureMemoryLoad::ShadowState, virt_cast<virt_addr>(data.addr), 0x800 * 4);
         break;
      }
      case IT_OPCODE::LOAD_BOOL_CONST:
      {
         decaf_abort("Unsupported LOAD_BOOL_CONST");
         auto data = read<LoadControlConst>(reader);
         scanLoadRegisters(latte::Register::BoolConstRegisterBase, data.addr, data.values);
         break;
      }
      case IT_OPCODE::LOAD_LOOP_CONST:
      {
         auto data = read<LoadControlConst>(reader);
         scanLoadRegisters(latte::Register::LoopConstRegisterBase, data.addr, data.values);
         trackMemory(CaptureMemoryLoad::ShadowState, virt_cast<virt_addr>(data.addr), 0x60 * 4);
         break;
      }
      case IT_OPCODE::LOAD_RESOURCE:
      {
         auto data = read<LoadControlConst>(reader);
         scanLoadRegisters(latte::Register::ResourceRegisterBase, data.addr, data.values);
         trackMemory(CaptureMemoryLoad::ShadowState, virt_cast<virt_addr>(data.addr), 0xD9E * 4);
         break;
      }
      case IT_OPCODE::LOAD_SAMPLER:
      {
         auto data = read<LoadControlConst>(reader);
         scanLoadRegisters(latte::Register::SamplerRegisterBase, data.addr, data.values);
         trackMemory(CaptureMemoryLoad::ShadowState, virt_cast<virt_addr>(data.addr), 0xA2 * 4);
         break;
      }
      case IT_OPCODE::LOAD_CTL_CONST:
      {
         decaf_abort("Unsupported LOAD_CTL_CONST");
         auto data = read<LoadControlConst>(reader);
         scanLoadRegisters(latte::Register::ControlRegisterBase, data.addr, data.values);
         break;
      }
      case IT_OPCODE::INDIRECT_BUFFER_PRIV:
      {
         auto data = read<IndirectBufferCall>(reader);
         trackMemory(CaptureMemoryLoad::CommandBuffer, cpu::translate(data.addr), data.size * 4u);
         scanCommandBuffer(reinterpret_cast<uint32_t *>(data.addr), data.size);
         break;
      }
      case IT_OPCODE::MEM_WRITE:
         break;
      case IT_OPCODE::EVENT_WRITE:
         break;
      case IT_OPCODE::EVENT_WRITE_EOP:
         break;
      case IT_OPCODE::PFP_SYNC_ME:
         break;
      case IT_OPCODE::STRMOUT_BASE_UPDATE:
         break;
      case IT_OPCODE::STRMOUT_BUFFER_UPDATE:
         break;
      case IT_OPCODE::NOP:
         break;
      case IT_OPCODE::SURFACE_SYNC:
      {
         auto data = read<SurfaceSync>(reader);
         trackMemory(CaptureMemoryLoad::SurfaceSync,
                     virt_addr { data.addr << 8 },
                     data.size << 8);
         break;
      }
      case IT_OPCODE::CONTEXT_CTL:
         break;
      default:
         gLog->debug("Unhandled pm4 packet type 3 opcode {}", header.opcode());
      }
   }

   // Returns true if the memory was written into pm4 stream
   bool
   trackMemory(CaptureMemoryLoad::MemoryType type,
               virt_addr addr,
               uint32_t size)
   {
      auto trackStart = addr;
      auto trackEnd = trackStart + size;
      auto addNewEntry = true;
      uint64_t hash[2] = { 0, 0 };
      auto useHash = HashAllMemory || (HashShadowState && type == CaptureMemoryLoad::ShadowState);

      if (!addr || size == 0) {
         return false;
      }

      if (useHash) {
         MurmurHash3_x64_128(virt_cast<void *>(addr).getRawPointer(), size, 0, hash);
      }

      for (auto &mem : mRecordedMemory) {
         if (trackStart < mem.start || trackStart > mem.end) {
            // Not in this block!
            continue;
         }

         if (trackEnd <= mem.end) {
            // Current memory is completely contained within an already tracked block
            if (useHash) {
               // If hash is enabled, then we do not write if hash matches
               if (hash[0] == mem.hash[0] && hash[1] == mem.hash[1]) {
                  return false;
               }
            } else if (type != CaptureMemoryLoad::CpuFlush) {
               // If hash is disabled, and this is NOT a flush, then do not write memory
               return false;
            }
         }

         mem.end = trackEnd;
         mem.hash[0] = hash[0];
         mem.hash[1] = hash[1];
         addNewEntry = false;
         break;
      }

      if (addNewEntry) {
         mRecordedMemory.emplace_back(RecordedMemory { trackStart, trackEnd, hash[0], hash[1] });
      }

      writeMemoryLoad(type, virt_cast<void *>(addr).getRawPointer(), size);
      return true;
   }

private:
   CaptureState mState = CaptureState::Disabled;
   std::mutex mMutex;
   std::ofstream mOut;
   std::vector<RecordedMemory> mRecordedMemory;
   std::array<uint32_t, 0x10000> mRegisters;
   size_t mCapturedFrames = 0;
   size_t mCaptureNumFrames = 0;
};

static Recorder
gRecorder;

bool
captureStartAtNextSwap()
{
   decaf_check(gRecorder.state() == CaptureState::Disabled);
   auto filename = std::string {};

   // Find an unused filename!
   for (auto i = 0u; i < 256u; ++i) {
      filename = fmt::format("trace{}.pm4", i);

      if (platform::fileExists(filename)) {
         continue;
      }

      break;
   }

   if (filename.empty()) {
      return false;
   }

   gLog->info("Starting pm4 trace as {}", filename);
   return gRecorder.requestStart(filename);
}

void
captureStopAtNextSwap()
{
   gRecorder.requestStop();
}

bool
captureNextFrame()
{
   gRecorder.setCaptureNumFrames(1);
   return captureStartAtNextSwap();
}

CaptureState
captureState()
{
   return gRecorder.state();
}

void
captureSwap()
{
   if (captureState() == CaptureState::WaitStartNextFrame) {
      internal::writePM4(DecafCapSyncRegisters {});
   }

   if (captureState() != CaptureState::Disabled) {
      gx2::GX2DrawDone();
      gRecorder.swap();
   }
}

void
captureCommandBuffer(CommandBuffer *buffer)
{
   if (captureState() == CaptureState::Enabled ||
       captureState() == CaptureState::WaitEndNextFrame) {
      gRecorder.commandBuffer(buffer);
   }
}

void
captureCpuFlush(void *buffer,
                uint32_t size)
{
   if (captureState() == CaptureState::Enabled ||
       captureState() == CaptureState::WaitEndNextFrame) {
      gRecorder.cpuFlush(buffer, size);
   }
}

void
captureGpuFlush(void *buffer,
                uint32_t size)
{
   if (captureState() == CaptureState::Enabled ||
       captureState() == CaptureState::WaitEndNextFrame) {
      gRecorder.gpuFlush(buffer, size);
   }
}

void
captureSyncGpuRegisters(const uint32_t *registers,
                        uint32_t size)
{
   gRecorder.syncRegisters(registers, size);
}

} // namespace cafe::gx2::internal
