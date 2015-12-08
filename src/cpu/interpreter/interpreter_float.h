#pragma once
#include "../state.h"

void
updateFEX_VX(ThreadState *state);

void
updateFX_FEX_VX(ThreadState *state, uint32_t oldValue);

void
updateFPSCR(ThreadState *state, uint32_t oldValue);

template<typename Type> void
updateFPRF(ThreadState *state, Type value);

void
updateFloatConditionRegister(ThreadState *state);

template<typename Type> Type
getFpr(ThreadState *state, unsigned fr);

template<> float
getFpr<float>(ThreadState *state, unsigned fr);

template<> double
getFpr<double>(ThreadState *state, unsigned fr);

void
setFpr(ThreadState *state, unsigned fr, float value);

void
setFpr(ThreadState *state, unsigned fr, double value);
