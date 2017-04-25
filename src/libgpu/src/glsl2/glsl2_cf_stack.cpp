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
insertElse(State &state)
{
   insertLineStart(state);
   state.out << "if (stack[stackIndex - 1] == Active) {";
   insertLineEnd(state);

   increaseIndent(state);
   insertLineStart(state);
   state.out << "activeMask = (activeMask == Active) ? InactiveBranch : Active;";
   insertLineEnd(state);
   decreaseIndent(state);

   insertLineStart(state);
   state.out << "}";
   insertLineEnd(state);
}


void
insertCond(State &state,
           latte::SQ_CF_COND cond)
{
   switch (cond) {
   case SQ_CF_COND::ACTIVE:
      state.out << "activeMask == Active";
      break;
   case SQ_CF_COND::ALWAYS_FALSE:
      state.out << "false";
      break;
   case SQ_CF_COND::CF_BOOL:
      throw translate_exception("Unimplemented SQ_CF_COND_BOOL");
   case SQ_CF_COND::CF_NOT_BOOL:
      throw translate_exception("Unimplemented SQ_CF_COND_NOT_BOOL");
   default:
      throw translate_exception(fmt::format("Invalid SQ_CF_COND {}", cond));
   }
}

void
condStart(State &state,
          SQ_CF_COND cond)
{
   insertLineStart(state);

   state.out << "if (";
   insertCond(state, cond);
   state.out << ") {";

   insertLineEnd(state);
   increaseIndent(state);
}

void
condElse(State &state)
{
   decreaseIndent(state);
   insertLineStart(state);
   state.out << "} else {";
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
