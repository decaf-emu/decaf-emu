#pragma once
#include <gsl.h>
#include <string>
#include "latte_shadir.h"

namespace latte
{

bool
decode(Shader &shader, const gsl::span<const uint8_t> &binary);

bool
blockify(Shader &shader);

bool
disassemble(Shader &shader, std::string &out);

void
debugDumpBlocks(Shader &shader, std::string &out);

// Internal, called by blockify
bool
linearify(Shader &shader);

} // namespace latte
