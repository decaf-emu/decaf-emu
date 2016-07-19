#pragma once
#include "../state.h"

double
ppc_estimate_reciprocal(double v);

void
updateFEX_VX(cpu::Core *state);

void
updateFX_FEX_VX(cpu::Core *state, uint32_t oldValue);

void
updateFPSCR(cpu::Core *state, uint32_t oldValue);

template<typename Type> void
updateFPRF(cpu::Core *state, Type value);

void
updateFloatConditionRegister(cpu::Core *state);

void
roundForMultiply(double *a, double *c);

template<typename Type> Type
getFpr(cpu::Core *state, unsigned fr);

template<> float
getFpr<float>(cpu::Core *state, unsigned fr);

template<> double
getFpr<double>(cpu::Core *state, unsigned fr);

void
setFpr(cpu::Core *state, unsigned fr, float value);

void
setFpr(cpu::Core *state, unsigned fr, double value);
