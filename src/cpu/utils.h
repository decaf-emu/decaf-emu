#pragma once
#include <cstdint>
#include "state.h"

uint32_t
getCRF(ThreadState *state, uint32_t field);

void
setCRF(ThreadState *state, uint32_t field, uint32_t value);

uint32_t
getCRB(ThreadState *state, uint32_t bit);

void
setCRB(ThreadState *state, uint32_t bit, uint32_t value);
