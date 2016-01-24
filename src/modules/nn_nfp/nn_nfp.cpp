#include "nn_nfp.h"

namespace nn
{

namespace nfp
{

void
Module::initialise()
{
}

void
Module::RegisterFunctions()
{
   registerInitFunctions();
   registerDetectionFunctions();
   registerSettingsFunctions();
}

} // namespace nfp

} // namespace nn
