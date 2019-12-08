#pragma once

#include <gsl.h>

namespace latte
{

std::string
disassemble(const gsl::span<const uint8_t> &binary, bool isSubroutine = false);

} // namespace latte
