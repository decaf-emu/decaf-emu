#pragma once
#include <array_view.h>
#include "gpu/pm4.h"

namespace gpu
{

namespace opengl
{

class Driver
{
public:
   void run();

private:
   void handlePacketType3(pm4::Packet3 header, gsl::array_view<uint32_t> data);
   void setContextReg(pm4::SetContextReg &data);

   template<typename Type>
   Type *getRegister(latte::Register::Value id)
   {
      return reinterpret_cast<Type *>(mRegisters[id]);
   }

private:
   volatile bool mRunning = true;
   uint32_t mRegisters[0x10000];
};

} // namespace opengl

} // namespace gpu
