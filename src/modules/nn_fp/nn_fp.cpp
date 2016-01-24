#include "nn_fp.h"

namespace nn
{

namespace fp
{

void
Module::initialise()
{
}

void
Module::RegisterFunctions()
{
   registerInitFunctions();
   registerFriendsFunctions();
}

} // namespace fp

} // namespace nn
