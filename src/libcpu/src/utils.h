#pragma once
#include <cstdint>
#include "state.h"

// TODO: Move these getCRX functions

uint32_t
getCRF(cpu::Core *state,
       uint32_t field);

void
setCRF(cpu::Core *state,
       uint32_t field,
       uint32_t value);

uint32_t
getCRB(cpu::Core *state,
       uint32_t bit);

void
setCRB(cpu::Core *state,
       uint32_t bit,
       uint32_t value);
