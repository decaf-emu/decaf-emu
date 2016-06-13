#include "nn_temp.h"

namespace nn
{

namespace temp
{

void
Module::initialise()
{
}

void
Module::RegisterFunctions()
{
   registerCoreFunctions();
   registerDirFunctions();
}

} // namespace temp

} // namespace nn
