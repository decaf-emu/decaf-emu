#ifdef DECAF_DX12

#include "clilog.h"
#include "common/decaf_assert.h"
#include "config.h"
#include "decafsdl_dx12.h"

DecafSDLDX12::DecafSDLDX12()
{
}

DecafSDLDX12::~DecafSDLDX12()
{
}

bool
DecafSDLDX12::initialise(int width, int height)
{

   // Setup decaf driver
   auto dxDriver = decaf::createDX12Driver();
   decaf_check(dxDriver);
   mDecafDriver = reinterpret_cast<decaf::DX12Driver*>(dxDriver);

   return true;
}

void
DecafSDLDX12::shutdown()
{
}

void
DecafSDLDX12::renderFrame(float tv[4], float drc[4])
{
}

decaf::GraphicsDriver *
DecafSDLDX12::getDecafDriver()
{
   return mDecafDriver;
}

#endif // DECAF_DX12
