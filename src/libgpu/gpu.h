#pragma once
#include <cstdint>

namespace gpu
{

using FlipCallbackFn = void(*)();

void
setFlipCallback(FlipCallbackFn callback);

} // namespace gpu
