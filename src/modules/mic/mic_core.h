#pragma once
#include "types.h"
#include "utils/be_val.h"

using MICHandle = uint32_t;

MICHandle
MICInit(MICHandle type, void *, void *, be_val<uint32_t> *result);
