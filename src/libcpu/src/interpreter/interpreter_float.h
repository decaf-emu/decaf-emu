#pragma once
#include <common/floatutils.h>
#include "../state.h"

template<typename Type>
static inline bool
possibleUnderflow(Type v)
{
   auto bits = get_float_bits(v);
   return bits.exponent == 1 && bits.mantissa == 0;
}

float
ppc_estimate_reciprocal(float v);

double
ppc_estimate_reciprocal_root(double v);

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

float
roundFMAResultToSingle(double result, double a, double b, double c);

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
