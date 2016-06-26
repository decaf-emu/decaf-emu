#pragma once
#include "glsl2_translate.h"

namespace glsl2
{

void
insertPush(State &state,
           unsigned count = 1);

void
insertPop(State &state,
          unsigned count = 1);

void
condStart(State &state,
          latte::SQ_CF_COND cond);

void
condEnd(State &state);

} // namespace glsl2
