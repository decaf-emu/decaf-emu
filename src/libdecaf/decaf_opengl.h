#pragma once

#ifndef DECAF_NOGL

namespace decaf
{

#include "decaf_graphics.h"

class OpenGLDriver : public GraphicsDriver
{
public:
   using SwapFunction = std::function<void(unsigned int, unsigned int)>;

   virtual ~OpenGLDriver()
   {
   }

   virtual void getSwapBuffers(unsigned int *tv, unsigned int *drc) = 0;
   virtual void syncPoll(const SwapFunction &swapFunc) = 0;
};

} // namespace decaf

#endif // DECAF_NOGL
