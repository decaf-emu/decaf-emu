#pragma once
#include "systemtypes.h"

using ProcUISaveCallback = wfunc_ptr<void>;
using ProcUISaveCallbackEx = wfunc_ptr<void>;

void
ProcUIInit(ProcUISaveCallback saveCallback);

void
ProcUIInitEx(ProcUISaveCallbackEx saveCallbackEx, void *arg);

uint32_t
ProcUIProcessMessages(BOOL unk1);

uint32_t
ProcUISubProcessMessages(BOOL unk1);
