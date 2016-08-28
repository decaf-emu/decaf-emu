#include "decaf_pm4replay.h"
#include "gpu_utilities.h"
#include "latte_constants.h"
#include "pm4_buffer.h"
#include "pm4_capture.h"
#include "pm4_format.h"
#include "pm4_packets.h"
#include "pm4_reader.h"
#include <array>
#include <common/byte_swap.h>
#include <common/log.h>
#include <common/murmur3.h>
#include <fstream>
#include <gsl.h>
#include <vector>

/**
 * THIS IS AN UNFINISHED EXPERIMENTAL FEATURE
 *
 * Known Issues
 * - We should be a bit more clever about updating LOAD_* memory ranges, these are
 *   the ones used for context state so often change, however only a few values in
 *   these memory ranges actually change, and uploading the whole buffer when a small
 *   part has changed can lead to unnecessarirly large traces.
 *
 * - Currently we hash every single piece of memory the GPU will ever read, every draw
 *   this is probably toooo much. This was important for detecting changes to the LOAD_*
 *   context state shadowing memory, because there is not an GX2Invalidate call on
 *   context state because the GPU is the one which edits the memory of it. This
 *   combined with the previous issue means we really need a custom handler for shadow
 *   state memory, rather than using the flush based tracking everything else uses.
 *
 * - We should probably use some cheap compression algorithm such as snappy to save
 *   even more space.
 *
 * - We seem to not upload the full data for a texture, bytes seem to be missing chopped
 *   off the end, there is probably a bug with surface size calculation.
 */


using decaf::pm4::CaptureMagic;
using decaf::pm4::CapturePacket;
using decaf::pm4::CaptureMemoryLoad;

namespace pm4
{

class Recorder
{
   struct RecordedMemory
   {
      uint32_t start;
      uint32_t end;
      uint64_t hash[2];
   };

public:
   bool
   open(const std::string &path)
   {
      std::unique_lock<std::mutex> lock { mMutex };
      mOut.open(path, std::fstream::binary);

      if (!mOut.is_open()) {
         return false;
      }

      mOut.write(CaptureMagic.data(), CaptureMagic.size());
      mRegisters.fill(0);
      mRecordedMemory.clear();
      mEnabled = true;
      return true;
   }

   bool
   enabled() const
   {
      return mEnabled;
   }

   void
   pause()
   {
      std::unique_lock<std::mutex> lock { mMutex };
      mEnabled = false;
   }

   void
   resume()
   {
      std::unique_lock<std::mutex> lock { mMutex };
      mEnabled = true;
   }

   void
   stop()
   {
      std::unique_lock<std::mutex> lock { mMutex };
      mEnabled = false;
      mOut.close();
   }

   void
   commandBuffer(pm4::Buffer *buffer)
   {
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
      std::unique_lock<std::mutex> lock { mMutex };
      auto addr = mem::untranslate(buffer);

      if (!trackMemory(addr, size)) {
         // Seeing as this is a flush, we need to write the memory load regardless
         // of whether we have already written the memory before
         writeMemoryLoad(buffer, size);
      }
   }

   void
   gpuFlush(void *buffer,
            uint32_t size)
   {
      // Not sure if we need to do something here...
   }

private:
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
   writeMemoryLoad(void *buffer,
                   uint32_t size)
   {
      CaptureMemoryLoad load;
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
         auto header = pm4::Header::get(buffer[pos]);
         auto size = size_t { 0u };

         switch (header.type()) {
         case pm4::Header::Type0:
         {
            auto header0 = pm4::type0::Header::get(header.value);
            size = header0.count() + 1;

            decaf_check(pos + size < bufferSize);
            scanType0(header0, gsl::as_span(&buffer[pos + 1], size));
            break;
         }
         case pm4::Header::Type3:
         {
            auto header3 = pm4::type3::Header::get(header.value);
            size = header3.size() + 1;

            decaf_check(pos + size < bufferSize);
            scanType3(header3, gsl::as_span(&buffer[pos + 1], size));
            break;
         }
         case pm4::Header::Type2:
         {
            // This is a filler packet, like a "nop", ignore it
            break;
         }
         case pm4::Header::Type1:
         default:
            gLog->error("Invalid packet header type {}, header = 0x{:08X}", header.type(), header.value);
            size = bufferSize;
            break;
         }

         pos += size + 1;
      }
   }

   void
   scanType0(pm4::type0::Header header,
             const gsl::span<uint32_t> &data)
   {
   }

   uint32_t
   getSurfaceBytes(uint32_t pitch,
                     uint32_t height,
                     uint32_t depth,
                     latte::SQ_TEX_DIM dim,
                     latte::SQ_DATA_FORMAT format)
   {
      uint32_t numPixels = 0;

      switch (dim) {
      case latte::SQ_TEX_DIM_1D:
         numPixels = pitch;
         break;
      case latte::SQ_TEX_DIM_2D:
         numPixels = pitch * height;
         break;
      case latte::SQ_TEX_DIM_2D_ARRAY:
         numPixels = pitch * height * depth;
         break;
      case latte::SQ_TEX_DIM_CUBEMAP:
         numPixels = pitch * height * 6;
         break;
      case latte::SQ_TEX_DIM_3D:
         numPixels = pitch * height * depth;
         break;
      case latte::SQ_TEX_DIM_1D_ARRAY:
         numPixels = pitch * height;
         break;
      default:
         decaf_abort(fmt::format("Unsupported texture dim: {}", dim));
      }

      if (format >= latte::FMT_BC1 && format <= latte::FMT_BC5) {
         numPixels /= 4 * 4;
      }

      auto bitsPerPixel = getDataFormatBitsPerElement(format);
      return numPixels * bitsPerPixel / 8;
   }

   void
   trackSurface(uint32_t baseAddress,
                uint32_t pitch,
                uint32_t height,
                uint32_t depth,
                latte::SQ_TEX_DIM dim,
                latte::SQ_DATA_FORMAT format,
                latte::SQ_TILE_MODE tileMode)
   {
      if (!baseAddress || !pitch || !height) {
         return;
      }

      // Adjust address for swizzling
      if (tileMode >= latte::SQ_TILE_MODE_TILED_2D_THIN1) {
         baseAddress &= ~(0x800 - 1);
      } else {
         baseAddress &= ~(0x100 - 1);
      }

      // Calculate size
      auto size = getSurfaceBytes(pitch, height, depth, dim, format);

      // Track that badboy
      trackMemory(baseAddress, size);
   }

   void
   trackColorBuffer(latte::CB_COLOR0_BASE cb_color_base,
                    latte::CB_COLOR0_SIZE cb_color_size,
                    latte::CB_COLOR0_INFO cb_color_info)
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
      auto tileMode = getArrayModeTileMode(cb_color_info.ARRAY_MODE());

      // Disabled for now, because it's a pointless upload
      //trackSurface(addr, pitch, height, 1, latte::SQ_TEX_DIM_2D, format, tileMode);
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
      auto tileMode = getArrayModeTileMode(db_depth_info.ARRAY_MODE());

      // Disabled for now, because it's a pointless upload
      //trackSurface(addr, pitch, height, 1, latte::SQ_TEX_DIM_2D, format, tileMode);
   }

   void
   trackActiveShaders()
   {
      auto pgm_start_fs = getRegister<latte::SQ_PGM_START_FS>(latte::Register::SQ_PGM_START_FS);
      auto pgm_size_fs = getRegister<latte::SQ_PGM_SIZE_FS>(latte::Register::SQ_PGM_SIZE_FS);
      trackMemory(pgm_start_fs.PGM_START() << 8, pgm_size_fs.PGM_SIZE() << 3);

      auto pgm_start_vs = getRegister<latte::SQ_PGM_START_VS>(latte::Register::SQ_PGM_START_VS);
      auto pgm_size_vs = getRegister<latte::SQ_PGM_SIZE_VS>(latte::Register::SQ_PGM_SIZE_VS);
      trackMemory(pgm_start_vs.PGM_START() << 8, pgm_size_vs.PGM_SIZE() << 3);

      auto pgm_start_ps = getRegister<latte::SQ_PGM_START_PS>(latte::Register::SQ_PGM_START_PS);
      auto pgm_size_ps = getRegister<latte::SQ_PGM_SIZE_PS>(latte::Register::SQ_PGM_SIZE_PS);
      trackMemory(pgm_start_ps.PGM_START() << 8, pgm_size_ps.PGM_SIZE() << 3);

      auto pgm_start_gs = getRegister<latte::SQ_PGM_START_GS>(latte::Register::SQ_PGM_START_GS);
      auto pgm_size_gs = getRegister<latte::SQ_PGM_SIZE_GS>(latte::Register::SQ_PGM_SIZE_GS);
      trackMemory(pgm_start_gs.PGM_START() << 8, pgm_size_gs.PGM_SIZE() << 3);
   }

   void
   trackActiveAttribBuffers()
   {
      for (auto i = 0u; i < latte::MaxAttributes; ++i) {
         auto resourceOffset = (latte::SQ_VS_ATTRIB_RESOURCE_0 + i) * 7;
         auto sq_vtx_constant_word0 = getRegister<latte::SQ_VTX_CONSTANT_WORD0_N>(latte::Register::SQ_VTX_CONSTANT_WORD0_0 + 4 * resourceOffset);
         auto sq_vtx_constant_word1 = getRegister<latte::SQ_VTX_CONSTANT_WORD1_N>(latte::Register::SQ_VTX_CONSTANT_WORD1_0 + 4 * resourceOffset);

         trackMemory(sq_vtx_constant_word0.BASE_ADDRESS(), sq_vtx_constant_word1.SIZE() + 1);
      }
   }

   void
   trackActiveUniforms()
   {
      for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
         auto sq_alu_const_cache_vs = getRegister<uint32_t>(latte::Register::SQ_ALU_CONST_CACHE_VS_0 + 4 * i);
         auto sq_alu_const_buffer_size_vs = getRegister<uint32_t>(latte::Register::SQ_ALU_CONST_BUFFER_SIZE_VS_0 + 4 * i);

         trackMemory(sq_alu_const_cache_vs << 8, sq_alu_const_buffer_size_vs << 8);
      }
   }

   void
   trackActiveTextures()
   {
      for (auto i = 0; i < latte::MaxTextures; ++i) {
         auto resourceOffset = (latte::SQ_PS_TEX_RESOURCE_0 + i) * 7;
         auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_TEX_RESOURCE_WORD0_0 + 4 * resourceOffset);
         auto sq_tex_resource_word1 = getRegister<latte::SQ_TEX_RESOURCE_WORD1_N>(latte::Register::SQ_TEX_RESOURCE_WORD1_0 + 4 * resourceOffset);
         auto sq_tex_resource_word2 = getRegister<latte::SQ_TEX_RESOURCE_WORD2_N>(latte::Register::SQ_TEX_RESOURCE_WORD2_0 + 4 * resourceOffset);
         auto sq_tex_resource_word3 = getRegister<latte::SQ_TEX_RESOURCE_WORD3_N>(latte::Register::SQ_TEX_RESOURCE_WORD3_0 + 4 * resourceOffset);
         auto sq_tex_resource_word4 = getRegister<latte::SQ_TEX_RESOURCE_WORD4_N>(latte::Register::SQ_TEX_RESOURCE_WORD4_0 + 4 * resourceOffset);
         auto sq_tex_resource_word5 = getRegister<latte::SQ_TEX_RESOURCE_WORD5_N>(latte::Register::SQ_TEX_RESOURCE_WORD5_0 + 4 * resourceOffset);
         auto sq_tex_resource_word6 = getRegister<latte::SQ_TEX_RESOURCE_WORD6_N>(latte::Register::SQ_TEX_RESOURCE_WORD6_0 + 4 * resourceOffset);

         auto baseAddress = sq_tex_resource_word2.BASE_ADDRESS() << 8;
         auto pitch = (sq_tex_resource_word0.PITCH() + 1) * 8;
         auto height = sq_tex_resource_word1.TEX_HEIGHT() + 1;
         auto depth = sq_tex_resource_word1.TEX_DEPTH() + 1;
         auto format = sq_tex_resource_word1.DATA_FORMAT();
         auto tileMode = sq_tex_resource_word0.TILE_MODE();
         auto dim = sq_tex_resource_word0.DIM();

         trackSurface(baseAddress, pitch, height, depth, dim, format, tileMode);
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
                     be_val<uint32_t> *src,
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
   scanType3(pm4::type3::Header header,
             const gsl::span<uint32_t> &data)
   {
      pm4::PacketReader reader { data };

      switch (header.opcode()) {
      case pm4::type3::DECAF_COPY_COLOR_TO_SCAN:
      {
         auto data = pm4::read<pm4::DecafCopyColorToScan>(reader);
         trackColorBuffer(data.cb_color_base, data.cb_color_size, data.cb_color_info);
         break;
      }
      case pm4::type3::DECAF_SWAP_BUFFERS:
         // Nothing to track!
         break;
      case pm4::type3::DECAF_CLEAR_COLOR:
      {
         auto data = pm4::read<pm4::DecafClearColor>(reader);
         trackColorBuffer(data.cb_color_base, data.cb_color_size, data.cb_color_info);
         break;
      }
      case pm4::type3::DECAF_CLEAR_DEPTH_STENCIL:
      {
         auto data = pm4::read<pm4::DecafClearDepthStencil>(reader);
         trackDepthBuffer(data.db_depth_base, data.db_depth_size, data.db_depth_info);
         break;
      }
      case pm4::type3::DECAF_SET_BUFFER:
         // Nothing to track!
         break;
      case pm4::type3::DECAF_DEBUGMARKER:
         // Nothing to track!
         break;
      case pm4::type3::DECAF_OSSCREEN_FLIP:
         decaf_abort("pm4 capture not enabled for OSScreen api");
         break;
      case pm4::type3::DECAF_COPY_SURFACE:
      {
         auto data = pm4::read<pm4::DecafCopySurface>(reader);
         trackSurface(data.srcImage, data.srcPitch, data.srcHeight, data.srcDepth, data.srcDim, data.srcFormat, data.srcTileMode);
         break;
      }
      case pm4::type3::DECAF_SET_SWAP_INTERVAL:
         // Nothing to track!
         break;
      case pm4::type3::DRAW_INDEX_AUTO:
         trackReadyDraw();
         break;
      case pm4::type3::DRAW_INDEX_2:
      {
         auto data = pm4::read<pm4::DrawIndex2>(reader);
         auto vgt_dma_index_type = getRegister<latte::VGT_DMA_INDEX_TYPE>(latte::Register::VGT_DMA_INDEX_TYPE);
         auto indexByteSize = 4u;

         if (vgt_dma_index_type.INDEX_TYPE() == latte::VGT_INDEX_16) {
            indexByteSize = 2u;
         }

         trackMemory(data.addr.getAddress(), data.count * indexByteSize);
         trackReadyDraw();
         break;
      }
      case pm4::type3::DRAW_INDEX_IMMD:
         trackReadyDraw();
         break;
      case pm4::type3::INDEX_TYPE:
      {
         auto data = pm4::read<pm4::IndexType>(reader);
         mRegisters[latte::Register::VGT_DMA_INDEX_TYPE / 4] = data.type.value;
         break;
      }
      case pm4::type3::NUM_INSTANCES:
      {
         auto data = pm4::read<pm4::NumInstances>(reader);
         mRegisters[latte::Register::VGT_DMA_NUM_INSTANCES / 4] = data.count;
         break;
      }
      case pm4::type3::SET_ALU_CONST:
      {
         auto data = pm4::read<pm4::SetAluConsts>(reader);

         for (auto i = 0u; i < data.values.size(); ++i) {
            scanSetRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
         }

         break;
      }
      case pm4::type3::SET_CONFIG_REG:
      {
         auto data = pm4::read<pm4::SetConfigRegs>(reader);

         for (auto i = 0u; i < data.values.size(); ++i) {
            scanSetRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
         }

         break;
      }
      case pm4::type3::SET_CONTEXT_REG:
      {
         auto data = pm4::read<pm4::SetContextRegs>(reader);

         for (auto i = 0u; i < data.values.size(); ++i) {
            scanSetRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
         }

         break;
      }
      case pm4::type3::SET_CTL_CONST:
      {
         auto data = pm4::read<pm4::SetControlConstants>(reader);

         for (auto i = 0u; i < data.values.size(); ++i) {
            scanSetRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
         }

         break;
      }
      case pm4::type3::SET_LOOP_CONST:
      {
         auto data = pm4::read<pm4::SetLoopConsts>(reader);

         for (auto i = 0u; i < data.values.size(); ++i) {
            scanSetRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
         }

         break;
      }
      case pm4::type3::SET_SAMPLER:
      {
         auto data = pm4::read<pm4::SetSamplers>(reader);

         for (auto i = 0u; i < data.values.size(); ++i) {
            scanSetRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
         }

         break;
      }
      case pm4::type3::SET_RESOURCE:
      {
         auto data = pm4::read<pm4::SetResources>(reader);
         auto id = latte::Register::ResourceRegisterBase + (4 * data.id);

         for (auto i = 0u; i < data.values.size(); ++i) {
            scanSetRegister(static_cast<latte::Register>(id + i * 4), data.values[i]);
         }

         break;
      }
      case pm4::type3::LOAD_CONFIG_REG:
      {
         auto data = pm4::read<pm4::LoadControlConst>(reader);
         scanLoadRegisters(latte::Register::ConfigRegisterBase, data.addr, data.values);
         trackMemory(data.addr.getAddress(), 0xB00 * 4);
         break;
      }
      case pm4::type3::LOAD_CONTEXT_REG:
      {
         auto data = pm4::read<pm4::LoadControlConst>(reader);
         scanLoadRegisters(latte::Register::ContextRegisterBase, data.addr, data.values);
         trackMemory(data.addr.getAddress(), 0x400 * 4);
         break;
      }
      case pm4::type3::LOAD_ALU_CONST:
      {
         auto data = pm4::read<pm4::LoadControlConst>(reader);
         scanLoadRegisters(latte::Register::AluConstRegisterBase, data.addr, data.values);
         trackMemory(data.addr.getAddress(), 0x800 * 4);
         break;
      }
      case pm4::type3::LOAD_BOOL_CONST:
      {
         decaf_abort("Unsupported LOAD_BOOL_CONST");
         auto data = pm4::read<pm4::LoadControlConst>(reader);
         scanLoadRegisters(latte::Register::BoolConstRegisterBase, data.addr, data.values);
         break;
      }
      case pm4::type3::LOAD_LOOP_CONST:
      {
         auto data = pm4::read<pm4::LoadControlConst>(reader);
         scanLoadRegisters(latte::Register::LoopConstRegisterBase, data.addr, data.values);
         trackMemory(data.addr.getAddress(), 0x60 * 4);
         break;
      }
      case pm4::type3::LOAD_RESOURCE:
      {
         auto data = pm4::read<pm4::LoadControlConst>(reader);
         scanLoadRegisters(latte::Register::ResourceRegisterBase, data.addr, data.values);
         trackMemory(data.addr.getAddress(), 0xD9E * 4);
         break;
      }
      case pm4::type3::LOAD_SAMPLER:
      {
         auto data = pm4::read<pm4::LoadControlConst>(reader);
         scanLoadRegisters(latte::Register::SamplerRegisterBase, data.addr, data.values);
         trackMemory(data.addr.getAddress(), 0xA2 * 4);
         break;
      }
      case pm4::type3::LOAD_CTL_CONST:
      {
         decaf_abort("Unsupported LOAD_CTL_CONST");
         auto data = pm4::read<pm4::LoadControlConst>(reader);
         scanLoadRegisters(latte::Register::ControlRegisterBase, data.addr, data.values);
         break;
      }
      case pm4::type3::INDIRECT_BUFFER_PRIV:
      {
         auto data = pm4::read<pm4::IndirectBufferCall>(reader);
         trackMemory(data.addr.getAddress(), data.size * 4u);
         scanCommandBuffer(reinterpret_cast<uint32_t *>(data.addr.get()), data.size);
         break;
      }
      case pm4::type3::MEM_WRITE:
         break;
      case pm4::type3::EVENT_WRITE:
         break;
      case pm4::type3::EVENT_WRITE_EOP:
         break;
      case pm4::type3::PFP_SYNC_ME:
         break;
      case pm4::type3::STRMOUT_BASE_UPDATE:
         break;
      case pm4::type3::STRMOUT_BUFFER_UPDATE:
         break;
      case pm4::type3::NOP:
         break;
      case pm4::type3::SURFACE_SYNC:
      {
         auto data = pm4::read<pm4::SurfaceSync>(reader);
         trackMemory(data.addr << 8, data.size << 8);
         break;
      }
      case pm4::type3::CONTEXT_CTL:
         break;
      default:
         gLog->debug("Unhandled pm4 packet type 3 opcode {}", header.opcode());
      }
   }

   // Returns true if the memory was written into pm4 stream
   bool
   trackMemory(uint32_t addr,
               uint32_t size)
   {
      auto trackStart = addr;
      auto trackEnd = trackStart + size;
      auto addNewEntry = true;
      uint64_t hash[2] = { 0, 0 };

      if (addr == 0 || size == 0) {
         return false;
      }

      MurmurHash3_x64_128(mem::translate(addr), size, 0, hash);

      for (auto &mem : mRecordedMemory) {
         if (trackStart < mem.start || trackStart > mem.end) {
            // Not in this block!
            continue;
         }

         if (trackEnd > mem.end) {
            // Starts in this block, but goes past the end, so adjust the block size
            mem.end = trackEnd;
         } else if (hash[0] == mem.hash[0] && hash[1] == mem.hash[1]) {
            // Memory matches, does not need to be written to pm4 trace
            return false;
         }

         mem.hash[0] = hash[0];
         mem.hash[1] = hash[1];
         addNewEntry = false;
         break;
      }

      if (addNewEntry) {
         mRecordedMemory.emplace_back(RecordedMemory { trackStart, trackEnd, hash[0], hash[1] });
      }

      writeMemoryLoad(mem::translate(addr), size);
      return true;
   }

private:
   bool mEnabled = false;
   std::mutex mMutex;
   std::ofstream mOut;
   std::vector<RecordedMemory> mRecordedMemory;
   std::array<uint32_t, 0x10000> mRegisters;
};

static Recorder
gRecorder;

void
captureStart(const std::string &path)
{
   decaf_check(!gRecorder.enabled());
   gRecorder.open(path);
}

void
captureStop()
{
   gRecorder.stop();
}

void
capturePause()
{
   gRecorder.pause();
}

void
captureResume()
{
   gRecorder.resume();
}

bool
captureEnabled()
{
   return gRecorder.enabled();
}

void
captureCommandBuffer(pm4::Buffer *buffer)
{
   if (captureEnabled()) {
      gRecorder.commandBuffer(buffer);
   }
}

void
captureCpuFlush(void *buffer,
                uint32_t size)
{
   if (captureEnabled()) {
      gRecorder.cpuFlush(buffer, size);
   }
}

void
captureGpuFlush(void *buffer,
                uint32_t size)
{
   if (captureEnabled()) {
      gRecorder.gpuFlush(buffer, size);
   }
}

} // namespace pm4
