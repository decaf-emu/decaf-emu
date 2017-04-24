#include "decaf_graphics.h"
#include <common/decaf_assert.h>

namespace decaf
{

static gpu::GraphicsDriver *
sGraphicsDriver = nullptr;

void
setGraphicsDriver(gpu::GraphicsDriver *driver)
{
   sGraphicsDriver = driver;
}

gpu::GraphicsDriver *
getGraphicsDriver()
{
   return sGraphicsDriver;
}

} // namespace decaf
