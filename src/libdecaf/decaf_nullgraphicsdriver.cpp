#include "decaf_nullgraphicsdriver.h"
#include "gpu/opengl/opengl_driver.h"
#include "gpu/commandqueue.h"

namespace decaf
{

NullGraphicsDriver::~NullGraphicsDriver()
{
}

void
NullGraphicsDriver::run()
{
   mRunning = true;

   while (mRunning) {
      auto buffer = gpu::unqueueCommandBuffer();

      if (!buffer) {
         continue;
      }

      gpu::retireCommandBuffer(buffer);
   }
}

void
NullGraphicsDriver::stop()
{
   mRunning = false;

   // Wake the GPU thread
   gpu::awaken();
}

float
NullGraphicsDriver::getAverageFPS()
{
   return 0.0f;
}

void
NullGraphicsDriver::handleDCFlush(uint32_t addr, uint32_t size)
{
}

OpenGLDriver *
createGLDriver()
{
   return new gpu::opengl::GLDriver();
}

GraphicsDriver *
createNullGraphicsDriver()
{
   return new NullGraphicsDriver();
}

} // namespace decaf
