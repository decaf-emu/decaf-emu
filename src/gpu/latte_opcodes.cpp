#include "latte_opcodes.h"

namespace latte
{

namespace cf
{

std::map<uint32_t, const char *> name = {
#define CF_INST(name, value) { value, #name },
#include "latte_opcodes_def.inl"
#undef CF_INST
};

} // namespace cf

namespace exp
{

std::map<uint32_t, const char *> name = {
#define EXP_INST(name, value) { value, #name },
#include "latte_opcodes_def.inl"
#undef EXP_INST
};

} // namespace exp

namespace alu
{

std::map<uint32_t, const char *> name = {
#define ALU_INST(name, value) { value, #name },
#include "latte_opcodes_def.inl"
#undef ALU_INST
};

std::map<uint32_t, Opcode> op2info = {
#define ALU_OP2(name, value, srcs, flags) { value, { value, srcs, static_cast<Opcode::Flags>(flags), #name } },
#include "latte_opcodes_def.inl"
#undef ALU_OP2
};

std::map<uint32_t, Opcode> op3info = {
#define ALU_OP3(name, value, srcs, flags) { value, { value, srcs, static_cast<Opcode::Flags>(flags), #name } },
#include "latte_opcodes_def.inl"
#undef ALU_OP3
};

} // namespace alu

namespace tex
{

std::map<uint32_t, const char *> name = {
#define TEX_INST(name, value) { value, #name },
#include "latte_opcodes_def.inl"
#undef TEX_INST
};

} // namespace tex

namespace vtx
{

std::map<uint32_t, const char *> name = {
#define VTX_INST(name, value) { value, #name },
#include "latte_opcodes_def.inl"
#undef VTX_INST
};

} // namespace vtx

namespace mem
{

std::map<uint32_t, const char *> name = {
#define MEM_INST(name, value) { value, #name },
#include "latte_opcodes_def.inl"
#undef MEM_INST
};

} // namespace mem

} // namespace latte
