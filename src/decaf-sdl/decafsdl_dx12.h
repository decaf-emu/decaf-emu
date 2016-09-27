#pragma once
#ifdef DECAF_DX12
#include "decafsdl_graphics.h"
#include "libdecaf/decaf.h"
#include "libdecaf/decaf_dx12.h"

class DecafSDLDX12 : public DecafSDLGraphics
{
public:
   DecafSDLDX12();
   ~DecafSDLDX12() override;

   bool
   initialise(int width, int height) override;

   void
   shutdown() override;

   void
   renderFrame(Viewport &tv, Viewport &drc) override;

   decaf::GraphicsDriver *
   getDecafDriver() override;

protected:
   std::thread mGraphicsThread;
   decaf::DX12Driver *mDecafDriver = nullptr;

};

#endif // DECAF_DX12
