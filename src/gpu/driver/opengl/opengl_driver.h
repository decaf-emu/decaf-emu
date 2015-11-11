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

private:
   volatile bool mRunning = true;
};

} // namespace opengl

} // namespace gpu
