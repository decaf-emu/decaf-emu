#pragma once

#ifdef DECAF_DX12

namespace decaf
{

#include "decaf_graphics.h"

class DX12Driver : public GraphicsDriver
{
public:
   virtual ~DX12Driver()
   {
   }

};

} // namespace decaf

#endif // DECAF_DX12