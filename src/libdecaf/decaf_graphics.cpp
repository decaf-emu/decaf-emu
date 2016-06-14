#include "decaf_graphics.h"

namespace decaf
{

static GraphicsDriver *
sGraphicsDriver = nullptr;

void
setGraphicsDriver(GraphicsDriver *driver)
{
   sGraphicsDriver = driver;
}

GraphicsDriver *
getGraphicsDriver()
{
   return sGraphicsDriver;
}

} // namespace decaf
