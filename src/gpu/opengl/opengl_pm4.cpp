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

      switch (header.type()) {
      case pm4::Header::Type3:
      {
         auto header3 = pm4::type3::Header::get(header.value);
         size = header3.size() + 1;

         if (pos + size >= buffer_size) {
            gLog->error("Invalid packet type3 size: {}", size);
         } else {
            handlePacketType3(header3, gsl::as_span(&buffer[pos + 1], size));
         }
         break;
      }
      case pm4::Header::Type0:
      {
         auto header0 = pm4::type0::Header::get(header.value);
         size = header0.count() + 1;

         if (pos + size >= buffer_size) {
            gLog->error("Invalid packet type0 size: {}", size);
         } else {
            handlePacketType0(header0, gsl::as_span(&buffer[pos + 1], size));
         }

         break;
      }
      case pm4::Header::Type2:
      {
         // Filler packet, ignore
         break;
      }
      case pm4::Header::Type1:
      default:
         gLog->error("Invalid packet header type {}, header = 0x{:08X}", header.type().get(), header.value);
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
   case pm4::type3::DECAF_SET_CONTEXT_STATE:
      decafSetContextState(pm4::read<pm4::DecafSetContextState>(reader));
      break;
   case pm4::type3::DRAW_INDEX_AUTO:
      drawIndexAuto(pm4::read<pm4::DrawIndexAuto>(reader));
      break;
   case pm4::type3::DRAW_INDEX_2:
      drawIndex2(pm4::read<pm4::DrawIndex2>(reader));
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
   case pm4::type3::EVENT_WRITE_EOP:
      eventWriteEOP(pm4::read<pm4::EventWriteEOP>(reader));
      break;
   default:
      gLog->debug("Unhandled pm4 packet type 3 opcode {}", header.opcode().get());
   }
}

void GLDriver::setAluConsts(const pm4::SetAluConsts &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setConfigRegs(const pm4::SetConfigRegs &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setContextRegs(const pm4::SetContextRegs &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setControlConstants(const pm4::SetControlConstants &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setLoopConsts(const pm4::SetLoopConsts &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setSamplers(const pm4::SetSamplers &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setResources(const pm4::SetResources &data)
{
   auto id = latte::Register::ResourceRegisterBase + (4 * data.id);

   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register>(id + i * 4), data.values[i]);
   }
}

void GLDriver::loadRegisters(latte::Register base, uint32_t *src, const gsl::span<std::pair<uint32_t, uint32_t>> &registers)
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
   loadRegisters(latte::Register::AluConstRegisterBase, data.addr, data.values);
}

void GLDriver::loadBoolConsts(const pm4::LoadBoolConst &data)
{
   loadRegisters(latte::Register::BoolConstRegisterBase, data.addr, data.values);
}

void GLDriver::loadConfigRegs(const pm4::LoadConfigReg &data)
{
   loadRegisters(latte::Register::ConfigRegisterBase, data.addr, data.values);
}

void GLDriver::loadContextRegs(const pm4::LoadContextReg &data)
{
   loadRegisters(latte::Register::ContextRegisterBase, data.addr, data.values);
}

void GLDriver::loadControlConstants(const pm4::LoadControlConst &data)
{
   loadRegisters(latte::Register::ControlRegisterBase, data.addr, data.values);
}

void GLDriver::loadLoopConsts(const pm4::LoadLoopConst &data)
{
   loadRegisters(latte::Register::LoopConstRegisterBase, data.addr, data.values);
}

void GLDriver::loadSamplers(const pm4::LoadSampler &data)
{
   loadRegisters(latte::Register::SamplerRegisterBase, data.addr, data.values);
}

void GLDriver::loadResources(const pm4::LoadResource &data)
{
   loadRegisters(latte::Register::ResourceRegisterBase, data.addr, data.values);
}

} // namespace opengl

} // namespace gpu
