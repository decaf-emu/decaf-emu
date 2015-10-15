#pragma once
#include "types.h"

void
KPADInit();

void
KPADInitEx(uint32_t unk1, uint32_t unk2);

uint32_t
KPADGetMplsWorkSize();

void
KPADSetMplsWorkarea(char *buffer);

void
KPADDisableDPD(uint32_t channel);
