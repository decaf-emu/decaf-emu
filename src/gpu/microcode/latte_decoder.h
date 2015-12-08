#pragma once
#include <gsl.h>
#include <string>
#include "latte_shadir.h"

namespace latte
{

bool
decode(Shader &shader, const gsl::span<const uint8_t> &binary);

bool
disassemble(Shader &shader, std::string &out);

void
debugDumpBlocks(Shader &shader, std::string &out);

// Internal
bool
linearify(Shader &shader);

bool
blockify(Shader &shader);

} // namespace latte
