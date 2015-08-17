#pragma once
#define NOMINMAX
#include <cstdint>
#include <spdlog/spdlog.h>

namespace latte
{

bool disassemble(fmt::MemoryWriter &out, uint8_t *binary, uint32_t size);

} // namespace latte
