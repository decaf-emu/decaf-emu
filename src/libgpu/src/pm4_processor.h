#pragma once
#include "latte/latte_pm4_commands.h"
#include "gpu_ringbuffer.h"

#include <array>
#include <common/byte_swap_array.h>
#include <libcpu/pointer.h>
#include <vector>

using namespace latte::pm4;

constexpr int MaxPm4IndirectDepth = 6;

// TODO: Once the OpenGL backend is removed from our code, we can remove
// the mNoRegisterNotify below, and the associated code from it.  Note
// the comment in loadRegisters as well!

class Pm4Processor
{
protected:
   Pm4Processor(bool noRegisterNotify = false)
      : mNoRegisterNotify(noRegisterNotify)
   {
   }

   virtual void decafSetBuffer(const DecafSetBuffer &data) = 0;
   virtual void decafCopyColorToScan(const DecafCopyColorToScan &data) = 0;
   virtual void decafSwapBuffers(const DecafSwapBuffers &data) = 0;
   virtual void decafCapSyncRegisters(const DecafCapSyncRegisters &data) = 0;
   virtual void decafClearColor(const DecafClearColor &data) = 0;
   virtual void decafClearDepthStencil(const DecafClearDepthStencil &data) = 0;
   virtual void decafOSScreenFlip(const DecafOSScreenFlip &data) = 0;
   virtual void decafCopySurface(const DecafCopySurface &data) = 0;
   virtual void decafExpandColorBuffer(const DecafExpandColorBuffer &data) = 0;
   virtual void drawIndexAuto(const DrawIndexAuto &data) = 0;
   virtual void drawIndex2(const DrawIndex2 &data) = 0;
   virtual void drawIndexImmd(const DrawIndexImmd &data) = 0;
   virtual void memWrite(const MemWrite &data) = 0;
   virtual void eventWrite(const EventWrite &data) = 0;
   virtual void eventWriteEOP(const EventWriteEOP &data) = 0;
   virtual void pfpSyncMe(const PfpSyncMe &data) = 0;
   virtual void setPredication(const SetPredication &data) = 0;
   virtual void streamOutBaseUpdate(const StreamOutBaseUpdate &data) = 0;
   virtual void streamOutBufferUpdate(const StreamOutBufferUpdate &data) = 0;
   virtual void surfaceSync(const SurfaceSync &data) = 0;

   void handlePacketType0(HeaderType0 header, const gsl::span<uint32_t> &data);
   void handlePacketType3(HeaderType3 header, const gsl::span<uint32_t> &data);
   void nopPacket(const Nop &data);
   void indirectBufferCall(const IndirectBufferCall &data);
   void indirectBufferCallPriv(const IndirectBufferCallPriv &data);
   void indexType(const IndexType &data);
   void numInstances(const NumInstances &data);
   void contextControl(const ContextControl &data);
   void copyDw(const CopyDw &data);

   virtual void
   applyRegister(latte::Register reg) = 0;

   void setAluConsts(const SetAluConsts &data);
   void setConfigRegs(const SetConfigRegs &data);
   void setContextRegs(const SetContextRegs &data);
   void setControlConstants(const SetControlConstants &data);
   void setLoopConsts(const SetLoopConsts &data);
   void setSamplers(const SetSamplers &data);
   void setResources(const SetResources &data);
   void shadowWrite(phys_ptr<uint32_t> address, const gsl::span<uint32_t> &registers);
   void setRegisters(latte::Register base, const gsl::span<uint32_t> &values);

   void loadAluConsts(const LoadAluConst &data);
   void loadBoolConsts(const LoadBoolConst &data);
   void loadConfigRegs(const LoadConfigReg &data);
   void loadContextRegs(const LoadContextReg &data);
   void loadControlConstants(const LoadControlConst &data);
   void loadLoopConsts(const LoadLoopConst &data);
   void loadSamplers(const LoadSampler &data);
   void loadResources(const latte::pm4::LoadResource &data); // Thanks Windows!
   void loadRegisters(latte::Register base,
                      phys_addr address,
                      const gsl::span<std::pair<uint32_t, uint32_t>> &registers);

   void runCommandBuffer(const gpu::ringbuffer::Buffer &buffer);

   uint32_t *byteSwapRegValues(uint32_t *values, size_t numValues)
   {
      return reinterpret_cast<uint32_t*>(byte_swap_to_scratch<uint32_t>(
         values, static_cast<uint32_t>(numValues) * sizeof(uint32_t), mRegisterScratch));
   }

   template<typename Type>
   Type getRegister(uint32_t id)
   {
      decaf_check(id != latte::Register::VGT_STRMOUT_DRAW_OPAQUE_BUFFER_FILLED_SIZE);

      static_assert(sizeof(Type) == 4, "Register storage must be a uint32_t");
      return *reinterpret_cast<Type *>(&mRegisters[id / 4]);
   }

   phys_addr getRegisterAddr(uint32_t id)
   {
      if (id == latte::Register::VGT_STRMOUT_DRAW_OPAQUE_BUFFER_FILLED_SIZE) {
         return mRegAddr_VGT_STRMOUT_DRAW_OPAQUE_BUFFER_FILLED_SIZE;
      }
      decaf_abort("Unsupported register address fetch");
   }

   bool mNoRegisterNotify = false;
   latte::ShadowState mShadowState;
   std::array<uint32_t, 0x10000> mRegisters = { 0 };
   phys_addr mRegAddr_VGT_STRMOUT_DRAW_OPAQUE_BUFFER_FILLED_SIZE = phys_addr { 0 };

   std::vector<uint8_t> mRegisterScratch;
   std::array<std::vector<uint8_t>, MaxPm4IndirectDepth> mSwapScratch;
   uint32_t mSwapScratchDepth = 0;
};
