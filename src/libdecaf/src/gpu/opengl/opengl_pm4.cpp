#include "opengl_driver.h"
#include "gpu/pm4_reader.h"

namespace gpu
{

namespace opengl
{

void
GLDriver::indirectBufferCall(const pm4::IndirectBufferCall &data)
{
   auto buffer = reinterpret_cast<uint32_t*>(data.addr.get());
   runCommandBuffer(buffer, data.size);
}

void
GLDriver::runCommandBuffer(uint32_t *buffer, uint32_t buffer_size)
{
   std::vector<uint32_t> swapped;
   swapped.resize(buffer_size);

   for (auto i = 0u; i < buffer_size; ++i) {
      swapped[i] = byte_swap(buffer[i]);
   }

   buffer = swapped.data();

   for (auto pos = 0u; pos < buffer_size; ) {
      auto header = *reinterpret_cast<pm4::Header *>(&buffer[pos]);
      auto size = 0u;

      if (buffer[pos] == 0) {
         break;
      }

      runRemoteThreadTasks();

      switch (header.type()) {
      case pm4::Header::Type3:
      {
         auto header3 = pm4::type3::Header::get(header.value);
         size = header3.size() + 1;

         decaf_check(pos + size <= buffer_size);
         handlePacketType3(header3, gsl::as_span(&buffer[pos + 1], size));
         break;
      }
      case pm4::Header::Type0:
      {
         auto header0 = pm4::type0::Header::get(header.value);
         size = header0.count() + 1;

         decaf_check(pos + size <= buffer_size);
         handlePacketType0(header0, gsl::as_span(&buffer[pos + 1], size));
         break;
      }
      case pm4::Header::Type2:
      {
         // Filler packet, ignore
         break;
      }
      case pm4::Header::Type1:
      default:
         gLog->error("Invalid packet header type {}, header = 0x{:08X}", header.type(), header.value);
         pos = buffer_size;
         break;
      }

      pos += size + 1;
   }
}

void
GLDriver::handlePacketType0(pm4::type0::Header header, const gsl::span<uint32_t> &data)
{
   auto base = header.baseIndex();

   for (auto i = 0; i < data.size(); ++i) {
      auto index = base + i;
      // Set mRegisters[base + i];
      gLog->info("Type0 set register 0x{:08X} = 0x{:08X}", index, data[i]);
   }
}

void
GLDriver::handlePacketType3(pm4::type3::Header header, const gsl::span<uint32_t> &data)
{
   pm4::PacketReader reader { data };

   switch (header.opcode()) {
   case pm4::type3::DECAF_COPY_COLOR_TO_SCAN:
      decafCopyColorToScan(pm4::read<pm4::DecafCopyColorToScan>(reader));
      break;
   case pm4::type3::DECAF_SWAP_BUFFERS:
      decafSwapBuffers(pm4::read<pm4::DecafSwapBuffers>(reader));
      break;
   case pm4::type3::DECAF_CLEAR_COLOR:
      decafClearColor(pm4::read<pm4::DecafClearColor>(reader));
      break;
   case pm4::type3::DECAF_CLEAR_DEPTH_STENCIL:
      decafClearDepthStencil(pm4::read<pm4::DecafClearDepthStencil>(reader));
      break;
   case pm4::type3::DECAF_SET_BUFFER:
      decafSetBuffer(pm4::read<pm4::DecafSetBuffer>(reader));
      break;
   case pm4::type3::DECAF_DEBUGMARKER:
      decafDebugMarker(pm4::read<pm4::DecafDebugMarker>(reader));
      break;
   case pm4::type3::DECAF_OSSCREEN_FLIP:
      decafOSScreenFlip(pm4::read<pm4::DecafOSScreenFlip>(reader));
      break;
   case pm4::type3::DECAF_COPY_SURFACE:
      decafCopySurface(pm4::read<pm4::DecafCopySurface>(reader));
      break;
   case pm4::type3::DECAF_SET_SWAP_INTERVAL:
      decafSetSwapInterval(pm4::read<pm4::DecafSetSwapInterval>(reader));
      break;
   case pm4::type3::DRAW_INDEX_AUTO:
      drawIndexAuto(pm4::read<pm4::DrawIndexAuto>(reader));
      break;
   case pm4::type3::DRAW_INDEX_2:
      drawIndex2(pm4::read<pm4::DrawIndex2>(reader));
      break;
   case pm4::type3::DRAW_INDEX_IMMD:
      drawIndexImmd(pm4::read<pm4::DrawIndexImmd>(reader));
      break;
   case pm4::type3::INDEX_TYPE:
      indexType(pm4::read<pm4::IndexType>(reader));
      break;
   case pm4::type3::NUM_INSTANCES:
      numInstances(pm4::read<pm4::NumInstances>(reader));
      break;
   case pm4::type3::SET_ALU_CONST:
      setAluConsts(pm4::read<pm4::SetAluConsts>(reader));
      break;
   case pm4::type3::SET_CONFIG_REG:
      setConfigRegs(pm4::read<pm4::SetConfigRegs>(reader));
      break;
   case pm4::type3::SET_CONTEXT_REG:
      setContextRegs(pm4::read<pm4::SetContextRegs>(reader));
      break;
   case pm4::type3::SET_CTL_CONST:
      setControlConstants(pm4::read<pm4::SetControlConstants>(reader));
      break;
   case pm4::type3::SET_LOOP_CONST:
      setLoopConsts(pm4::read<pm4::SetLoopConsts>(reader));
      break;
   case pm4::type3::SET_SAMPLER:
      setSamplers(pm4::read<pm4::SetSamplers>(reader));
      break;
   case pm4::type3::SET_RESOURCE:
      setResources(pm4::read<pm4::SetResources>(reader));
      break;
   case pm4::type3::LOAD_CONFIG_REG:
      loadConfigRegs(pm4::read<pm4::LoadConfigReg>(reader));
      break;
   case pm4::type3::LOAD_CONTEXT_REG:
      loadContextRegs(pm4::read<pm4::LoadContextReg>(reader));
      break;
   case pm4::type3::LOAD_ALU_CONST:
      loadAluConsts(pm4::read<pm4::LoadAluConst>(reader));
      break;
   case pm4::type3::LOAD_BOOL_CONST:
      loadBoolConsts(pm4::read<pm4::LoadBoolConst>(reader));
      break;
   case pm4::type3::LOAD_LOOP_CONST:
      loadLoopConsts(pm4::read<pm4::LoadLoopConst>(reader));
      break;
   case pm4::type3::LOAD_RESOURCE:
      loadResources(pm4::read<pm4::LoadResource>(reader));
      break;
   case pm4::type3::LOAD_SAMPLER:
      loadSamplers(pm4::read<pm4::LoadSampler>(reader));
      break;
   case pm4::type3::LOAD_CTL_CONST:
      loadControlConstants(pm4::read<pm4::LoadControlConst>(reader));
      break;
   case pm4::type3::INDIRECT_BUFFER_PRIV:
      indirectBufferCall(pm4::read<pm4::IndirectBufferCall>(reader));
      break;
   case pm4::type3::MEM_WRITE:
      memWrite(pm4::read<pm4::MemWrite>(reader));
      break;
   case pm4::type3::EVENT_WRITE:
      eventWrite(pm4::read<pm4::EventWrite>(reader));
      break;
   case pm4::type3::EVENT_WRITE_EOP:
      eventWriteEOP(pm4::read<pm4::EventWriteEOP>(reader));
      break;
   case pm4::type3::PFP_SYNC_ME:
      pfpSyncMe(pm4::read<pm4::PfpSyncMe>(reader));
      break;
   case pm4::type3::STRMOUT_BASE_UPDATE:
      streamOutBaseUpdate(pm4::read<pm4::StreamOutBaseUpdate>(reader));
      break;
   case pm4::type3::STRMOUT_BUFFER_UPDATE:
      streamOutBufferUpdate(pm4::read<pm4::StreamOutBufferUpdate>(reader));
      break;
   case pm4::type3::NOP:
      nopPacket(pm4::read<pm4::Nop>(reader));
      break;
   case pm4::type3::SURFACE_SYNC:
      surfaceSync(pm4::read<pm4::SurfaceSync>(reader));
      break;
   case pm4::type3::CONTEXT_CTL:
      contextControl(pm4::read<pm4::ContextControl>(reader));
      break;
   default:
      gLog->debug("Unhandled pm4 packet type 3 opcode {}", header.opcode());
   }
}

void GLDriver::contextControl(const pm4::ContextControl &data)
{
   mShadowState.LOAD_CONTROL = data.LOAD_CONTROL;
   mShadowState.SHADOW_ENABLE = data.SHADOW_ENABLE;
}

void GLDriver::setAluConsts(const pm4::SetAluConsts &data)
{
   decaf_check(data.id >= latte::Register::AluConstRegisterBase);
   decaf_check(data.id < latte::Register::AluConstRegisterEnd);
   auto offset = (data.id - latte::Register::AluConstRegisterBase) / 4;

   if (mShadowState.SHADOW_ENABLE.ENABLE_ALU_CONST() && mShadowState.ALU_CONST_BASE) {
      auto base = &mShadowState.ALU_CONST_BASE[offset];

      for (auto i = 0u; i < data.values.size(); ++i) {
         base[i] = data.values[i];
      }
   }

   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }

   auto firstUniform = offset / 4;
   auto lastUniform = (offset + data.values.size() - 1) / 4;

   for (auto i = firstUniform / 16; i <= lastUniform / 16; ++i) {
      mLastUniformUpdate[i] = mUniformUpdateGen;
   }
}

void GLDriver::setConfigRegs(const pm4::SetConfigRegs &data)
{
   if (mShadowState.SHADOW_ENABLE.ENABLE_CONFIG_REG() && mShadowState.CONFIG_REG_BASE) {
      decaf_check(data.id >= latte::Register::ConfigRegisterBase);
      decaf_check(data.id < latte::Register::ConfigRegisterEnd);
      auto offset = (data.id - latte::Register::ConfigRegisterBase) / 4;
      auto base = &mShadowState.CONFIG_REG_BASE[offset];

      for (auto i = 0u; i < data.values.size(); ++i) {
         base[i] = data.values[i];
      }
   }

   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setContextRegs(const pm4::SetContextRegs &data)
{
   if (mShadowState.SHADOW_ENABLE.ENABLE_CONTEXT_REG() && mShadowState.CONTEXT_REG_BASE) {
      decaf_check(data.id >= latte::Register::ContextRegisterBase);
      decaf_check(data.id < latte::Register::ContextRegisterEnd);
      auto offset = (data.id - latte::Register::ContextRegisterBase) / 4;
      auto base = &mShadowState.CONTEXT_REG_BASE[offset];

      for (auto i = 0u; i < data.values.size(); ++i) {
         base[i] = data.values[i];
      }
   }

   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setControlConstants(const pm4::SetControlConstants &data)
{
   if (mShadowState.SHADOW_ENABLE.ENABLE_CTL_CONST() && mShadowState.CTL_CONST_BASE) {
      decaf_check(data.id >= latte::Register::ControlRegisterBase);
      decaf_check(data.id < latte::Register::ControlRegisterEnd);
      auto offset = (data.id - latte::Register::ControlRegisterBase) / 4;
      auto base = &mShadowState.CTL_CONST_BASE[offset];

      for (auto i = 0u; i < data.values.size(); ++i) {
         base[i] = data.values[i];
      }
   }

   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setLoopConsts(const pm4::SetLoopConsts &data)
{
   if (mShadowState.SHADOW_ENABLE.ENABLE_LOOP_CONST() && mShadowState.LOOP_CONST_BASE) {
      decaf_check(data.id >= latte::Register::LoopConstRegisterBase);
      decaf_check(data.id < latte::Register::LoopConstRegisterEnd);
      auto offset = (data.id - latte::Register::LoopConstRegisterBase) / 4;
      auto base = &mShadowState.LOOP_CONST_BASE[offset];

      for (auto i = 0u; i < data.values.size(); ++i) {
         base[i] = data.values[i];
      }
   }

   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setSamplers(const pm4::SetSamplers &data)
{
   if (mShadowState.SHADOW_ENABLE.ENABLE_SAMPLER() && mShadowState.SAMPLER_CONST_BASE) {
      decaf_check(data.id >= latte::Register::SamplerRegisterBase);
      decaf_check(data.id < latte::Register::SamplerRegisterEnd);
      auto offset = (data.id - latte::Register::SamplerRegisterBase) / 4;
      auto base = &mShadowState.SAMPLER_CONST_BASE[offset];

      for (auto i = 0u; i < data.values.size(); ++i) {
         base[i] = data.values[i];
      }
   }

   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setResources(const pm4::SetResources &data)
{
   auto id = latte::Register::ResourceRegisterBase + (4 * data.id);

   if (mShadowState.SHADOW_ENABLE.ENABLE_RESOURCE() && mShadowState.RESOURCE_CONST_BASE) {
      auto base = &mShadowState.RESOURCE_CONST_BASE[data.id];

      for (auto i = 0u; i < data.values.size(); ++i) {
         base[i] = data.values[i];
      }
   }

   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(id + i * 4), data.values[i]);
   }
}

void GLDriver::loadRegisters(latte::Register base,
                             be_val<uint32_t> *src,
                             const gsl::span<std::pair<uint32_t, uint32_t>> &registers)
{
   for (auto &range : registers) {
      auto start = range.first;
      auto count = range.second;

      for (auto j = start; j < start + count; ++j) {
         setRegister(static_cast<latte::Register>(base + j * 4), src[j]);
      }
   }
}

void GLDriver::loadAluConsts(const pm4::LoadAluConst &data)
{
   if (mShadowState.LOAD_CONTROL.ENABLE_ALU_CONST()) {
      mShadowState.ALU_CONST_BASE = data.addr;
      loadRegisters(latte::Register::AluConstRegisterBase, data.addr, data.values);
   }
}

void GLDriver::loadBoolConsts(const pm4::LoadBoolConst &data)
{
   if (mShadowState.LOAD_CONTROL.ENABLE_BOOL_CONST()) {
      mShadowState.BOOL_CONST_BASE = data.addr;
      loadRegisters(latte::Register::BoolConstRegisterBase, data.addr, data.values);
   }
}

void GLDriver::loadConfigRegs(const pm4::LoadConfigReg &data)
{
   if (mShadowState.LOAD_CONTROL.ENABLE_CONFIG_REG()) {
      mShadowState.CONFIG_REG_BASE = data.addr;
      loadRegisters(latte::Register::ConfigRegisterBase, data.addr, data.values);
   }
}

void GLDriver::loadContextRegs(const pm4::LoadContextReg &data)
{
   if (mShadowState.LOAD_CONTROL.ENABLE_CONTEXT_REG()) {
      mShadowState.CONTEXT_REG_BASE = data.addr;
      loadRegisters(latte::Register::ContextRegisterBase, data.addr, data.values);
   }
}

void GLDriver::loadControlConstants(const pm4::LoadControlConst &data)
{
   if (mShadowState.LOAD_CONTROL.ENABLE_CTL_CONST()) {
      mShadowState.CTL_CONST_BASE = data.addr;
      loadRegisters(latte::Register::ControlRegisterBase, data.addr, data.values);
   }
}

void GLDriver::loadLoopConsts(const pm4::LoadLoopConst &data)
{
   if (mShadowState.LOAD_CONTROL.ENABLE_LOOP_CONST()) {
      mShadowState.LOOP_CONST_BASE = data.addr;
      loadRegisters(latte::Register::LoopConstRegisterBase, data.addr, data.values);
   }
}

void GLDriver::loadSamplers(const pm4::LoadSampler &data)
{
   if (mShadowState.LOAD_CONTROL.ENABLE_SAMPLER()) {
      mShadowState.SAMPLER_CONST_BASE = data.addr;
      loadRegisters(latte::Register::SamplerRegisterBase, data.addr, data.values);
   }
}

void GLDriver::loadResources(const pm4::LoadResource &data)
{
   if (mShadowState.LOAD_CONTROL.ENABLE_RESOURCE()) {
      mShadowState.RESOURCE_CONST_BASE = data.addr;
      loadRegisters(latte::Register::ResourceRegisterBase, data.addr, data.values);
   }
}

void GLDriver::nopPacket(const pm4::Nop &data)
{
   auto str = std::string { };

   if (data.strWords.size()) {
      for (auto i = 0u; i < data.strWords.size(); ++i) {
         auto word = data.strWords[i];

         for (auto c = 0u; c < 4; ++c) {
            auto chr = static_cast<char>((word >> (c * 8)) & 0xFF);

            if (!chr) {
               break;
            }

            str.push_back(chr);
         }
      }
   }

   if (false) {
      gLog->debug("NOP unk: {} str: {}", data.unk, str);
   }
}

} // namespace opengl

} // namespace gpu
