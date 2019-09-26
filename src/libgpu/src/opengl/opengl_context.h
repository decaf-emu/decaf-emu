#pragma once
#include "gpu_graphicsdriver.h"

#include <memory>
#include <utility>

namespace opengl
{

class GLContext
{
public:
   virtual ~GLContext() = default;

   virtual void makeCurrent() = 0;
   virtual void clearCurrent() = 0;
   virtual std::pair<int, int> getDimensions() = 0;
   virtual void setSwapInterval(int interval) = 0;
   virtual void swapBuffers() = 0;
   virtual std::unique_ptr<GLContext> createSharedContext() = 0;
};

std::unique_ptr<GLContext>
createContext(const gpu::WindowSystemInfo &wsi);

} // namespace opengl
