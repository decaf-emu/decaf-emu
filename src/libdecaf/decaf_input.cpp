#include "decaf_input.h"

namespace decaf
{

InputDriver *
sInputDriver = nullptr;

void
setInputDriver(InputDriver *driver)
{
   sInputDriver = driver;
}

InputDriver *
getInputDriver()
{
   return sInputDriver;
}

} // namespace input
