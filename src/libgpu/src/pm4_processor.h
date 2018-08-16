#pragma once
#include "latte/latte_pm4_commands.h"
#include <libcpu/pointer.h>

using namespace latte::pm4;

class Pm4Processor
{
protected:
   virtual void decafSetBuffer(const DecafSetBuffer &data) = 0;
   virtual void decafCopyColorToScan(const DecafCopyColorToScan &data) = 0;
   virtual void decafSwapBuffers(const DecafSwapBuffers &data) = 0;
   virtual void decafCapSyncRegisters(const DecafCapSyncRegisters &data) = 0;
   virtual void decafClearColor(const DecafClearColor &data) = 0;
   virtual void decafClearDepthStencil(const DecafClearDepthStencil &data) = 0;
   virtual void decafDebugMarker(const DecafDebugMarker &data) = 0;
   virtual void decafOSScreenFlip(const DecafOSScreenFlip &data) = 0;
   virtual void decafCopySurface(const DecafCopySurface &data) = 0;
   virtual void decafSetSwapInterval(const DecafSetSwapInterval &data) = 0;
   virtual void drawIndexAuto(const DrawIndexAuto &data) = 0;
   virtual void drawIndex2(const DrawIndex2 &data) = 0;
   virtual void drawIndexImmd(const DrawIndexImmd &data) = 0;
   virtual void memWrite(const MemWrite &data) = 0;
   virtual void eventWrite(const EventWrite &data) = 0;
   virtual void eventWriteEOP(const EventWriteEOP &data) = 0;
   virtual void pfpSyncMe(const PfpSyncMe &data) = 0;
   virtual void streamOutBaseUpdate(const StreamOutBaseUpdate &data) = 0;
   virtual void streamOutBufferUpdate(const StreamOutBufferUpdate &data) = 0;
   virtual void surfaceSync(const SurfaceSync &data) = 0;

   void handlePacketType0(HeaderType0 header, const gsl::span<uint32_t> &data);
   void handlePacketType3(HeaderType3 header, const gsl::span<uint32_t> &data);
   void nopPacket(const Nop &data);
   void indirectBufferCall(const IndirectBufferCall &data);
   void indexType(const IndexType &data);
   void numInstances(const NumInstances &data);
   void contextControl(const ContextControl &data);

   virtual void
   applyRegister(latte::Register reg) = 0;

   void setAluConsts(const SetAluConsts &data);
   void setConfigRegs(const SetConfigRegs &data);
   void setContextRegs(const SetContextRegs &data);
   void setControlConstants(const SetControlConstants &data);
   void setLoopConsts(const SetLoopConsts &data);
   void setSamplers(const SetSamplers &data);
   void setResources(const SetResources &data);

   void loadAluConsts(const LoadAluConst &data);
   void loadBoolConsts(const LoadBoolConst &data);
   void loadConfigRegs(const LoadConfigReg &data);
   void loadContextRegs(const LoadContextReg &data);
   void loadControlConstants(const LoadControlConst &data);
   void loadLoopConsts(const LoadLoopConst &data);
   void loadSamplers(const LoadSampler &data);
   void loadResources(const latte::pm4::LoadResource &data); // Thanks Windows!
   void loadRegisters(latte::Register base,
                      virt_ptr<uint32_t> src,
                      const gsl::span<std::pair<uint32_t, uint32_t>> &registers);

   void setRegister(latte::Register reg, uint32_t value);
   void runCommandBuffer(uint32_t *buffer, uint32_t size);

   template<typename Type>
   Type getRegister(uint32_t id)
   {
      static_assert(sizeof(Type) == 4, "Register storage must be a uint32_t");
      return *reinterpret_cast<Type *>(&mRegisters[id / 4]);
   }

   latte::ShadowState mShadowState;
   std::array<uint32_t, 0x10000> mRegisters;
};
