#include "latte/latte_pm4_reader.h"
#include "pm4_processor.h"

#include <common/log.h>
#include <libcpu/mmu.h>

void
Pm4Processor::indirectBufferCall(const IndirectBufferCall &data)
{
   auto buffer = reinterpret_cast<uint32_t *>(data.addr);
   runCommandBuffer(buffer, data.size);
}

void
Pm4Processor::runCommandBuffer(uint32_t *buffer, uint32_t buffer_size)
{
   std::vector<uint32_t> swapped;
   swapped.resize(buffer_size);

   for (auto i = 0u; i < buffer_size; ++i) {
      swapped[i] = byte_swap(buffer[i]);
   }

   for (auto pos = 0u; pos < buffer_size; ) {
      auto header = *reinterpret_cast<Header *>(&swapped[pos]);
      auto size = 0u;

      if (swapped[pos] == 0) {
         break;
      }

      switch (header.type()) {
      case PacketType::Type3:
      {
         auto header3 = HeaderType3::get(header.value);
         size = header3.size() + 1;

         decaf_check(pos + size <= buffer_size);
         handlePacketType3(header3, gsl::make_span(&swapped[pos + 1], size));
         break;
      }
      case PacketType::Type0:
      {
         auto header0 = HeaderType0::get(header.value);
         size = header0.count() + 1;

         decaf_check(pos + size <= buffer_size);
         handlePacketType0(header0, gsl::make_span(&swapped[pos + 1], size));
         break;
      }
      case PacketType::Type2:
      {
         // Filler packet, ignore
         break;
      }
      case PacketType::Type1:
      default:
         gLog->error("Invalid packet header type {}, header = 0x{:08X}", header.type(), header.value);
         pos = buffer_size;
         break;
      }

      pos += size + 1;
   }
}

void
Pm4Processor::handlePacketType0(HeaderType0 header, const gsl::span<uint32_t> &data)
{
   auto base = header.baseIndex();

   for (auto i = 0; i < data.size(); ++i) {
      auto index = base + i;
      // Set mRegisters[base + i];
      gLog->info("Type0 set register 0x{:08X} = 0x{:08X}", index, data[i]);
   }
}

void
Pm4Processor::handlePacketType3(HeaderType3 header, const gsl::span<uint32_t> &data)
{
   PacketReader reader{ data };

   switch (header.opcode()) {
   case IT_OPCODE::DECAF_COPY_COLOR_TO_SCAN:
      decafCopyColorToScan(read<DecafCopyColorToScan>(reader));
      break;
   case IT_OPCODE::DECAF_SWAP_BUFFERS:
      decafSwapBuffers(read<DecafSwapBuffers>(reader));
      break;
   case IT_OPCODE::DECAF_CAP_SYNC_REGISTERS:
      decafCapSyncRegisters(read<DecafCapSyncRegisters>(reader));
      break;
   case IT_OPCODE::DECAF_CLEAR_COLOR:
      decafClearColor(read<DecafClearColor>(reader));
      break;
   case IT_OPCODE::DECAF_CLEAR_DEPTH_STENCIL:
      decafClearDepthStencil(read<DecafClearDepthStencil>(reader));
      break;
   case IT_OPCODE::DECAF_SET_BUFFER:
      decafSetBuffer(read<DecafSetBuffer>(reader));
      break;
   case IT_OPCODE::DECAF_DEBUGMARKER:
      decafDebugMarker(read<DecafDebugMarker>(reader));
      break;
   case IT_OPCODE::DECAF_OSSCREEN_FLIP:
      decafOSScreenFlip(read<DecafOSScreenFlip>(reader));
      break;
   case IT_OPCODE::DECAF_COPY_SURFACE:
      decafCopySurface(read<DecafCopySurface>(reader));
      break;
   case IT_OPCODE::DECAF_SET_SWAP_INTERVAL:
      decafSetSwapInterval(read<DecafSetSwapInterval>(reader));
      break;
   case IT_OPCODE::DRAW_INDEX_AUTO:
      drawIndexAuto(read<DrawIndexAuto>(reader));
      break;
   case IT_OPCODE::DRAW_INDEX_2:
      drawIndex2(read<DrawIndex2>(reader));
      break;
   case IT_OPCODE::DRAW_INDEX_IMMD:
      drawIndexImmd(read<DrawIndexImmd>(reader));
      break;
   case IT_OPCODE::INDEX_TYPE:
      indexType(read<IndexType>(reader));
      break;
   case IT_OPCODE::NUM_INSTANCES:
      numInstances(read<NumInstances>(reader));
      break;
   case IT_OPCODE::SET_ALU_CONST:
      setAluConsts(read<SetAluConsts>(reader));
      break;
   case IT_OPCODE::SET_CONFIG_REG:
      setConfigRegs(read<SetConfigRegs>(reader));
      break;
   case IT_OPCODE::SET_CONTEXT_REG:
      setContextRegs(read<SetContextRegs>(reader));
      break;
   case IT_OPCODE::SET_CTL_CONST:
      setControlConstants(read<SetControlConstants>(reader));
      break;
   case IT_OPCODE::SET_LOOP_CONST:
      setLoopConsts(read<SetLoopConsts>(reader));
      break;
   case IT_OPCODE::SET_SAMPLER:
      setSamplers(read<SetSamplers>(reader));
      break;
   case IT_OPCODE::SET_RESOURCE:
      setResources(read<SetResources>(reader));
      break;
   case IT_OPCODE::LOAD_CONFIG_REG:
      loadConfigRegs(read<LoadConfigReg>(reader));
      break;
   case IT_OPCODE::LOAD_CONTEXT_REG:
      loadContextRegs(read<LoadContextReg>(reader));
      break;
   case IT_OPCODE::LOAD_ALU_CONST:
      loadAluConsts(read<LoadAluConst>(reader));
      break;
   case IT_OPCODE::LOAD_BOOL_CONST:
      loadBoolConsts(read<LoadBoolConst>(reader));
      break;
   case IT_OPCODE::LOAD_LOOP_CONST:
      loadLoopConsts(read<LoadLoopConst>(reader));
      break;
   case IT_OPCODE::LOAD_RESOURCE:
      loadResources(read<latte::pm4::LoadResource>(reader));
      break;
   case IT_OPCODE::LOAD_SAMPLER:
      loadSamplers(read<LoadSampler>(reader));
      break;
   case IT_OPCODE::LOAD_CTL_CONST:
      loadControlConstants(read<LoadControlConst>(reader));
      break;
   case IT_OPCODE::INDIRECT_BUFFER_PRIV:
      indirectBufferCall(read<IndirectBufferCall>(reader));
      break;
   case IT_OPCODE::MEM_WRITE:
      memWrite(read<MemWrite>(reader));
      break;
   case IT_OPCODE::EVENT_WRITE:
      eventWrite(read<EventWrite>(reader));
      break;
   case IT_OPCODE::EVENT_WRITE_EOP:
      eventWriteEOP(read<EventWriteEOP>(reader));
      break;
   case IT_OPCODE::PFP_SYNC_ME:
      pfpSyncMe(read<PfpSyncMe>(reader));
      break;
   case IT_OPCODE::STRMOUT_BASE_UPDATE:
      streamOutBaseUpdate(read<StreamOutBaseUpdate>(reader));
      break;
   case IT_OPCODE::STRMOUT_BUFFER_UPDATE:
      streamOutBufferUpdate(read<StreamOutBufferUpdate>(reader));
      break;
   case IT_OPCODE::NOP:
      nopPacket(read<Nop>(reader));
      break;
   case IT_OPCODE::SURFACE_SYNC:
      surfaceSync(read<SurfaceSync>(reader));
      break;
   case IT_OPCODE::CONTEXT_CTL:
      contextControl(read<ContextControl>(reader));
      break;
   default:
      gLog->debug("Unhandled pm4 packet type 3 opcode {}", header.opcode());
   }
}

void Pm4Processor::nopPacket(const Nop &data)
{
   auto str = std::string{};

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

void
Pm4Processor::indexType(const IndexType &data)
{
   mRegisters[latte::Register::VGT_DMA_INDEX_TYPE / 4] = data.type.value;
}

void
Pm4Processor::numInstances(const NumInstances &data)
{
   mRegisters[latte::Register::VGT_DMA_NUM_INSTANCES / 4] = data.count;
}

void Pm4Processor::contextControl(const ContextControl &data)
{
   mShadowState.LOAD_CONTROL = data.LOAD_CONTROL;
   mShadowState.SHADOW_ENABLE = data.SHADOW_ENABLE;
}

void Pm4Processor::setAluConsts(const SetAluConsts &data)
{
   decaf_check(data.id >= latte::Register::AluConstRegisterBase);
   decaf_check(data.id < latte::Register::AluConstRegisterEnd);
   auto offset = (data.id - latte::Register::AluConstRegisterBase) / 4;

   if (mShadowState.SHADOW_ENABLE.ENABLE_ALU_CONST() && mShadowState.ALU_CONST_BASE) {
      auto base = mShadowState.ALU_CONST_BASE + offset;

      for (auto i = 0u; i < data.values.size(); ++i) {
         base[i] = data.values[i];
      }
   }

   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }
}

void Pm4Processor::setConfigRegs(const SetConfigRegs &data)
{
   if (mShadowState.SHADOW_ENABLE.ENABLE_CONFIG_REG() && mShadowState.CONFIG_REG_BASE) {
      decaf_check(data.id >= latte::Register::ConfigRegisterBase);
      decaf_check(data.id < latte::Register::ConfigRegisterEnd);
      auto offset = (data.id - latte::Register::ConfigRegisterBase) / 4;
      auto base = mShadowState.CONFIG_REG_BASE + offset;

      for (auto i = 0u; i < data.values.size(); ++i) {
         base[i] = data.values[i];
      }
   }

   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }
}

void Pm4Processor::setContextRegs(const SetContextRegs &data)
{
   if (mShadowState.SHADOW_ENABLE.ENABLE_CONTEXT_REG() && mShadowState.CONTEXT_REG_BASE) {
      decaf_check(data.id >= latte::Register::ContextRegisterBase);
      decaf_check(data.id < latte::Register::ContextRegisterEnd);
      auto offset = (data.id - latte::Register::ContextRegisterBase) / 4;
      auto base = mShadowState.CONTEXT_REG_BASE + offset;

      for (auto i = 0u; i < data.values.size(); ++i) {
         base[i] = data.values[i];
      }
   }

   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }
}

void Pm4Processor::setControlConstants(const SetControlConstants &data)
{
   if (mShadowState.SHADOW_ENABLE.ENABLE_CTL_CONST() && mShadowState.CTL_CONST_BASE) {
      decaf_check(data.id >= latte::Register::ControlRegisterBase);
      decaf_check(data.id < latte::Register::ControlRegisterEnd);
      auto offset = (data.id - latte::Register::ControlRegisterBase) / 4;
      auto base = mShadowState.CTL_CONST_BASE + offset;

      for (auto i = 0u; i < data.values.size(); ++i) {
         base[i] = data.values[i];
      }
   }

   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }
}

void Pm4Processor::setLoopConsts(const SetLoopConsts &data)
{
   if (mShadowState.SHADOW_ENABLE.ENABLE_LOOP_CONST() && mShadowState.LOOP_CONST_BASE) {
      decaf_check(data.id >= latte::Register::LoopConstRegisterBase);
      decaf_check(data.id < latte::Register::LoopConstRegisterEnd);
      auto offset = (data.id - latte::Register::LoopConstRegisterBase) / 4;
      auto base = mShadowState.LOOP_CONST_BASE + offset;

      for (auto i = 0u; i < data.values.size(); ++i) {
         base[i] = data.values[i];
      }
   }

   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }
}

void Pm4Processor::setSamplers(const SetSamplers &data)
{
   if (mShadowState.SHADOW_ENABLE.ENABLE_SAMPLER() && mShadowState.SAMPLER_CONST_BASE) {
      decaf_check(data.id >= latte::Register::SamplerRegisterBase);
      decaf_check(data.id < latte::Register::SamplerRegisterEnd);
      auto offset = (data.id - latte::Register::SamplerRegisterBase) / 4;
      auto base = mShadowState.SAMPLER_CONST_BASE + offset;

      for (auto i = 0u; i < data.values.size(); ++i) {
         base[i] = data.values[i];
      }
   }

   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }
}

void Pm4Processor::setResources(const SetResources &data)
{
   auto id = latte::Register::ResourceRegisterBase + (4 * data.id);

   if (mShadowState.SHADOW_ENABLE.ENABLE_RESOURCE() && mShadowState.RESOURCE_CONST_BASE) {
      auto base = mShadowState.RESOURCE_CONST_BASE + data.id;

      for (auto i = 0u; i < data.values.size(); ++i) {
         base[i] = data.values[i];
      }
   }

   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(id + i * 4), data.values[i]);
   }
}

void Pm4Processor::loadRegisters(latte::Register base,
   virt_ptr<uint32_t> src,
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

void Pm4Processor::loadAluConsts(const LoadAluConst &data)
{
   if (mShadowState.LOAD_CONTROL.ENABLE_ALU_CONST()) {
      mShadowState.ALU_CONST_BASE = data.addr;
      loadRegisters(latte::Register::AluConstRegisterBase, data.addr, data.values);
   }
}

void Pm4Processor::loadBoolConsts(const LoadBoolConst &data)
{
   if (mShadowState.LOAD_CONTROL.ENABLE_BOOL_CONST()) {
      mShadowState.BOOL_CONST_BASE = data.addr;
      loadRegisters(latte::Register::BoolConstRegisterBase, data.addr, data.values);
   }
}

void Pm4Processor::loadConfigRegs(const LoadConfigReg &data)
{
   if (mShadowState.LOAD_CONTROL.ENABLE_CONFIG_REG()) {
      mShadowState.CONFIG_REG_BASE = data.addr;
      loadRegisters(latte::Register::ConfigRegisterBase, data.addr, data.values);
   }
}

void Pm4Processor::loadContextRegs(const LoadContextReg &data)
{
   if (mShadowState.LOAD_CONTROL.ENABLE_CONTEXT_REG()) {
      mShadowState.CONTEXT_REG_BASE = data.addr;
      loadRegisters(latte::Register::ContextRegisterBase, data.addr, data.values);
   }
}

void Pm4Processor::loadControlConstants(const LoadControlConst &data)
{
   if (mShadowState.LOAD_CONTROL.ENABLE_CTL_CONST()) {
      mShadowState.CTL_CONST_BASE = data.addr;
      loadRegisters(latte::Register::ControlRegisterBase, data.addr, data.values);
   }
}

void Pm4Processor::loadLoopConsts(const LoadLoopConst &data)
{
   if (mShadowState.LOAD_CONTROL.ENABLE_LOOP_CONST()) {
      mShadowState.LOOP_CONST_BASE = data.addr;
      loadRegisters(latte::Register::LoopConstRegisterBase, data.addr, data.values);
   }
}

void Pm4Processor::loadSamplers(const LoadSampler &data)
{
   if (mShadowState.LOAD_CONTROL.ENABLE_SAMPLER()) {
      mShadowState.SAMPLER_CONST_BASE = data.addr;
      loadRegisters(latte::Register::SamplerRegisterBase, data.addr, data.values);
   }
}

void Pm4Processor::loadResources(const latte::pm4::LoadResource &data)
{
   if (mShadowState.LOAD_CONTROL.ENABLE_RESOURCE()) {
      mShadowState.RESOURCE_CONST_BASE = data.addr;
      loadRegisters(latte::Register::ResourceRegisterBase, data.addr, data.values);
   }
}

void
Pm4Processor::setRegister(latte::Register reg,
                          uint32_t value)
{
   decaf_check((reg % 4) == 0);
   auto isChanged = (value != mRegisters[reg / 4]);

   // Save to local registers
   mRegisters[reg / 4] = value;

   // Writing SQ_VTX_SEMANTIC_CLEAR has side effects, so process those
   if (reg == latte::Register::SQ_VTX_SEMANTIC_CLEAR) {
      for (auto i = 0u; i < 32; ++i) {
         if (value & (1 << i)) {
            setRegister(static_cast<latte::Register>(latte::Register::SQ_VTX_SEMANTIC_0 + i * 4), 0xffffffff);
         }
      }
   }

   // Apply changes directly to OpenGL state if appropriate
   if (isChanged) {
      applyRegister(reg);
   }
}
