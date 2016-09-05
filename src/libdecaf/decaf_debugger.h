#pragma once
#include <cstdint>

namespace decaf
{

namespace debugger
{

void
initialise();

#ifndef DECAF_NOGL

void
initialiseUiGL();

void
drawUiGL(uint32_t width, uint32_t height);

#endif // DECAF_NOGL

#ifdef DECAF_DX12

void
initialiseUiDX12();

void
drawUiDX12(uint32_t width, uint32_t height);

#endif

} // namespace debugger

} // namespace decaf
