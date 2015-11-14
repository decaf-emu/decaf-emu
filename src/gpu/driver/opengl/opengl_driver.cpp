#include "gpu/driver/commandqueue.h"
#include "opengl_driver.h"
#include "gpu/pm4_buffer.h"
#include "gpu/pm4_reader.h"
#include "gpu/latte_registers.h"
#include <glbinding/gl/gl.h>

namespace gpu
{

namespace opengl
{

void Driver::setContextReg(pm4::SetContextRegs &data)
{
   // Copy to local register store
   auto *dst = &mRegisters[data.id / 4];
   memcpy(dst, data.values.data(), data.values.size() * sizeof(uint32_t));

   // Shadow in memory

   // Perform OpenGL operation
   switch (data.id) {
   case latte::Register::CB_BLEND_CONTROL:
      //gl::glBlendFunc(src, dst);
      break;
   case latte::Register::CB_BLEND0_CONTROL:
   case latte::Register::CB_BLEND1_CONTROL:
   case latte::Register::CB_BLEND2_CONTROL:
   case latte::Register::CB_BLEND3_CONTROL:
   case latte::Register::CB_BLEND4_CONTROL:
   case latte::Register::CB_BLEND5_CONTROL:
   case latte::Register::CB_BLEND6_CONTROL:
   case latte::Register::CB_BLEND7_CONTROL:
      //gl::glBlendFunci(data.id - latte::Register::CB_BLEND0_CONTROL, src, dst);
      break;
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
