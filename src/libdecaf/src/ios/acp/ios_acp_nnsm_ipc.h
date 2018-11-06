#pragma once
#include "ios/ios_enum.h"
#include <string_view>

namespace ios::acp
{

Error
NNSM_RegisterServer(std::string_view device);

Error
NNSM_UnregisterServer(std::string_view device);

} // namespace ios::acp
