#include "glsl2_cf.h"

using namespace latte;

namespace glsl2
{

void
insertPush(State &state,
           unsigned count)
{
   for (auto i = 0u; i < count; ++i) {
      insertLineStart(state);
      state.out << "PUSH(stack, stackIndex, activeMask);";
      insertLineEnd(state);
   }
}

void
insertPop(State &state,
          unsigned count)
{
   for (auto i = 0u; i < count; ++i) {
      insertLineStart(state);
      state.out << "POP(stack, stackIndex, activeMask);";
      insertLineEnd(state);
   }
}

void
condStart(State &state,
          SQ_CF_COND cond)
{
   insertLineStart(state);

   switch (cond) {
   case SQ_CF_COND_ACTIVE:
      state.out << "if (activeMask) {";
      break;
   case SQ_CF_COND_FALSE:
      state.out << "if (!activeMask) {";
      break;
   case SQ_CF_COND_BOOL:
      throw std::logic_error("Unimplemented SQ_CF_COND_BOOL");
      break;
   case SQ_CF_COND_NOT_BOOL:
      throw std::logic_error("Unimplemented SQ_CF_COND_NOT_BOOL");
      break;
   default:
      throw std::logic_error(fmt::format("Unknown SQ_CF_COND {}", cond));
   }

   insertLineEnd(state);
   increaseIndent(state);
}

void
condEnd(State &state)
{
   decreaseIndent(state);
   insertLineStart(state);
   state.out << '}';
   insertLineEnd(state);
}

} // namespace glsl2
