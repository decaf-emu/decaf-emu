#include "nn_save.h"

namespace nn
{

namespace save
{

void
Module::initialise()
{
}

void
Module::RegisterFunctions()
{
   registerCoreFunctions();
   registerCmdFunctions();
}

} // namespace save

} // namespace nn
