#pragma once

#include "gpu/pm4_packets.h"

namespace gpu
{

class Pm4Processor
{
public:

protected:
   virtual void decafSetBuffer(const pm4::DecafSetBuffer &data) = 0;
   virtual void decafCopyColorToScan(const pm4::DecafCopyColorToScan &data) = 0;
   virtual void decafSwapBuffers(const pm4::DecafSwapBuffers &data) = 0;
   virtual void decafCapSyncRegisters(const pm4::DecafCapSyncRegisters &data) = 0;
   virtual void decafClearColor(const pm4::DecafClearColor &data) = 0;
   virtual void decafClearDepthStencil(const pm4::DecafClearDepthStencil &data) = 0;
   virtual void decafDebugMarker(const pm4::DecafDebugMarker &data) = 0;
   virtual void decafOSScreenFlip(const pm4::DecafOSScreenFlip &data) = 0;
   virtual void decafCopySurface(const pm4::DecafCopySurface &data) = 0;
   virtual void decafSetSwapInterval(const pm4::DecafSetSwapInterval &data) = 0;
   virtual void drawIndexAuto(const pm4::DrawIndexAuto &data) = 0;
   virtual void drawIndex2(const pm4::DrawIndex2 &data) = 0;
   virtual void drawIndexImmd(const pm4::DrawIndexImmd &data) = 0;
   virtual void memWrite(const pm4::MemWrite &data) = 0;
   virtual void eventWrite(const pm4::EventWrite &data) = 0;
   virtual void eventWriteEOP(const pm4::EventWriteEOP &data) = 0;
   virtual void pfpSyncMe(const pm4::PfpSyncMe &data) = 0;
   virtual void streamOutBaseUpdate(const pm4::StreamOutBaseUpdate &data) = 0;
   virtual void streamOutBufferUpdate(const pm4::StreamOutBufferUpdate &data) = 0;
   virtual void surfaceSync(const pm4::SurfaceSync &data) = 0;

   void handlePacketType0(pm4::type0::Header header, const gsl::span<uint32_t> &data);
   void handlePacketType3(pm4::type3::Header header, const gsl::span<uint32_t> &data);
   void nopPacket(const pm4::Nop &data);
   void indirectBufferCall(const pm4::IndirectBufferCall &data);
   void indexType(const pm4::IndexType &data);
   void numInstances(const pm4::NumInstances &data);
   void contextControl(const pm4::ContextControl &data);

   virtual void
   applyRegister(latte::Register reg) = 0;

   void setAluConsts(const pm4::SetAluConsts &data);
   void setConfigRegs(const pm4::SetConfigRegs &data);
   void setContextRegs(const pm4::SetContextRegs &data);
   void setControlConstants(const pm4::SetControlConstants &data);
   void setLoopConsts(const pm4::SetLoopConsts &data);
   void setSamplers(const pm4::SetSamplers &data);
   void setResources(const pm4::SetResources &data);

   void loadAluConsts(const pm4::LoadAluConst &data);
   void loadBoolConsts(const pm4::LoadBoolConst &data);
   void loadConfigRegs(const pm4::LoadConfigReg &data);
   void loadContextRegs(const pm4::LoadContextReg &data);
   void loadControlConstants(const pm4::LoadControlConst &data);
   void loadLoopConsts(const pm4::LoadLoopConst &data);
   void loadSamplers(const pm4::LoadSampler &data);
   void loadResources(const pm4::LoadResource &data);
   void loadRegisters(latte::Register base,
      be_val<uint32_t> *src,
      const gsl::span<std::pair<uint32_t, uint32_t>> &registers);

   void
   setRegister(latte::Register reg, uint32_t value);

   void
   runCommandBuffer(uint32_t *buffer,
                    uint32_t size);

   template<typename Type>
   Type getRegister(uint32_t id)
   {
      static_assert(sizeof(Type) == 4, "Register storage must be a uint32_t");
      return *reinterpret_cast<Type *>(&mRegisters[id / 4]);
   }


   latte::ShadowState mShadowState;
   std::array<uint32_t, 0x10000> mRegisters;

};

} // namespace gpu