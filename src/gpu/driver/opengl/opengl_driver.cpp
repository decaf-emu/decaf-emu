#include "gpu/driver/commandqueue.h"
#include "opengl_driver.h"
#include "gpu/pm4_buffer.h"
#include "gpu/pm4_reader.h"
#include "gpu/latte_registers.h"
#include "platform/platform_ui.h"
#include <glbinding/gl/gl.h>

namespace gpu
{

namespace opengl
{

// We need LOAD_REG and SET_REG to do same shit.

void Driver::checkActiveShader()
{
   auto pgm_start_fs = getRegister<latte::SQ_PGM_START_FS>(latte::Register::SQ_PGM_START_FS);
   auto pgm_start_vs = getRegister<latte::SQ_PGM_START_VS>(latte::Register::SQ_PGM_START_VS);
   auto pgm_start_ps = getRegister<latte::SQ_PGM_START_PS>(latte::Register::SQ_PGM_START_PS);

   if (mActiveShader &&
       mActiveShader->fetch && mActiveShader->fetch->pgm_start_fs.PGM_START != pgm_start_fs.PGM_START
       && mActiveShader->vertex && mActiveShader->vertex->pgm_start_vs.PGM_START != pgm_start_vs.PGM_START
       && mActiveShader->pixel && mActiveShader->pixel->pgm_start_ps.PGM_START != pgm_start_ps.PGM_START) {
      // OpenGL shader matches latte shader
      return;
   }

   auto fsProgram = make_virtual_ptr<void>(pgm_start_fs.PGM_START << 8);
   auto vsProgram = make_virtual_ptr<void>(pgm_start_vs.PGM_START << 8);
   auto psProgram = make_virtual_ptr<void>(pgm_start_ps.PGM_START << 8);

   // Update OpenGL shader
   auto &fetchShader = mFetchShaders[pgm_start_fs.PGM_START];
   auto &vertexShader = mVertexShaders[pgm_start_vs.PGM_START];
   auto &pixelShader = mPixelShaders[pgm_start_ps.PGM_START];
   auto &shader = mShaders[{ pgm_start_vs.PGM_START, pgm_start_ps.PGM_START }];

   if (shader.program != -1) {
      // TODO: bind shader
      return;
   }

   if (!fetchShader.parsed) {
      // Parse attrib stream
   }

   if (vertexShader.program == -1) {

   }

   if (pixelShader.program == -1) {

   }
}

void Driver::setRegister(latte::Register::Value reg, uint32_t value)
{
   // Save to my state
   mRegisters[reg / 4] = value;

   // TODO: Save to active context state shadow regs

   // For the following registers, we apply their state changes
   //   directly to the OpenGL context...
   switch (reg) {
   case latte::Register::CB_BLEND_CONTROL:
   case latte::Register::CB_BLEND0_CONTROL:
   case latte::Register::CB_BLEND1_CONTROL:
   case latte::Register::CB_BLEND2_CONTROL:
   case latte::Register::CB_BLEND3_CONTROL:
   case latte::Register::CB_BLEND4_CONTROL:
   case latte::Register::CB_BLEND5_CONTROL:
   case latte::Register::CB_BLEND6_CONTROL:
   case latte::Register::CB_BLEND7_CONTROL:
      // gl::something();
      break;
   }
}

void Driver::setContextReg(pm4::SetContextRegs &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register::Value>(data.id + i * 4), data.values[i]);
   }
}

void Driver::handlePacketType3(pm4::Packet3 header, gsl::array_view<uint32_t> data)
{
   pm4::PacketReader reader { data };

   switch (header.opcode) {
   case pm4::Opcode3::SET_CONTEXT_REG:
      setContextReg(pm4::read<pm4::SetContextRegs>(reader));
      break;
   }
}

void Driver::start()
{
   mRunning = true;
   mThread = std::thread(&Driver::run, this);
}

void Driver::run()
{
   while (mRunning) {
      auto buffer = gpu::unqueueCommandBuffer();

      for (auto pos = 0u; pos < buffer->curSize; ) {
         auto header = *reinterpret_cast<pm4::PacketHeader *>(&buffer->buffer[pos]);
         auto size = 0u;

         switch (header.type) {
         case pm4::PacketType::Type3:
         {
            auto header3 = pm4::Packet3 { header.value };
            size = header3.size + 1;
            //handlePacketType3(header3, { &buffer->buffer[pos + 1], size });
            break;
         }
         case pm4::PacketType::Type0:
         case pm4::PacketType::Type1:
         case pm4::PacketType::Type2:
         default:
            throw std::logic_error("What the fuck son");
         }

         pos += size + 1;
      }

      gpu::retireCommandBuffer(buffer);
   }
}

} // namespace opengl

} // namespace gpu
