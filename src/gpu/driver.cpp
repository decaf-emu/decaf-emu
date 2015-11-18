#include "driver.h"

static gpu::Driver *
gActiveDriver = nullptr;

namespace gpu
{

namespace driver
{

void
create(Driver *driver)
{
   gActiveDriver = driver;
}

void
start()
{
   gActiveDriver->start();
}

void
destroy()
{
   delete gActiveDriver;
   gActiveDriver = nullptr;
}

void
setupWindow()
{
   gActiveDriver->setupWindow();
}

void
setTvDisplay(size_t width, size_t height)
{
   gActiveDriver->setTvDisplay(width, height);
}

void
setDrcDisplay(size_t width, size_t height)
{
   gActiveDriver->setDrcDisplay(width, height);
}

} // namespace driver

} // namespace gpu
