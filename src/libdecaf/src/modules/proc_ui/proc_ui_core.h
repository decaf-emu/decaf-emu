#pragma once
#include "common/types.h"
#include "ppcutils/wfunc_ptr.h"
#include "proc_ui_enum.h"

namespace proc_ui
{

// Unknown parameters.
using ProcUISaveCallback = wfunc_ptr<void>;
using ProcUISaveCallbackEx = wfunc_ptr<uint32_t, void*>;
using ProcUICallback = wfunc_ptr<uint32_t, void *>;

void
ProcUIInit(ProcUISaveCallback saveCallback);

void
ProcUIInitEx(ProcUISaveCallbackEx saveCallbackEx,
             void *arg);

ProcUIStatus
ProcUIProcessMessages(BOOL block);

ProcUIStatus
ProcUISubProcessMessages(BOOL block);

void
ProcUIRegisterCallback(ProcUICallbackType type,
                       ProcUICallback callback,
                       void *param,
                       uint32_t unk);

void
ProcUISetMEM1Storage(void *buffer,
                     uint32_t size);

} // namespace proc_ui
