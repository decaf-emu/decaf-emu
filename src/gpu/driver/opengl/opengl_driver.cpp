#include "gpu/driver/commandqueue.h"
#include "opengl_driver.h"
#include "gpu/pm4_buffer.h"
#include "gpu/pm4_reader.h"

namespace gpu
{

namespace opengl
{

void Driver::setContextReg(pm4::SetContextReg &data)
{
   switch (data.id) {
   case pm4::Register::Blend0Control:
   case pm4::Register::Blend1Control:
   case pm4::Register::Blend2Control:
   case pm4::Register::Blend3Control:
   case pm4::Register::Blend4Control:
   case pm4::Register::Blend5Control:
   case pm4::Register::Blend6Control:
   case pm4::Register::Blend7Control:
      break;
   }
}

void Driver::handlePacketType3(pm4::Packet3 header, gsl::array_view<uint32_t> data)
{
   pm4::PacketReader reader { data };

   switch (header.opcode) {
   case pm4::Opcode3::SET_CONTEXT_REG:
      setContextReg(pm4::read<pm4::SetContextReg>(reader));
      break;
   }
}

void Driver::run()
{
   while (mRunning) {
      auto buffer = gpu::unqueueCommandBuffer();

      for (auto pos = 0u; pos < buffer->curSize; ) {
         auto header = *reinterpret_cast<pm4::PacketHeader *>(&buffer->buffer[pos]);
         auto size = 1u;

         switch (header.type) {
         case pm4::PacketType::Type3:
         {
            auto header3 = pm4::Packet3 { header.value };
            size = header3.size;
            handlePacketType3(header3, { &buffer->buffer[pos], size });
            break;
         }
         case pm4::PacketType::Type0:
         case pm4::PacketType::Type1:
         case pm4::PacketType::Type2:
         default:
            throw std::logic_error("What the fuck son");
         }

         pos += size;
      }

      gpu::retireCommandBuffer(buffer);
   }
}

} // namespace opengl

} // namespace gpu
