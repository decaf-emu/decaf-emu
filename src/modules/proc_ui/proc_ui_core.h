#pragma once
#include "types.h"
#include "utils/wfunc_ptr.h"

// Unknown parameters.
using ProcUISaveCallback = wfunc_ptr<void>;
using ProcUISaveCallbackEx = wfunc_ptr<uint32_t, void*>;
using ProcUICallback = wfunc_ptr<uint32_t, void *>;

namespace ProcUICallbackType
{
enum Type
{
};
}

void
ProcUIInit(ProcUISaveCallback saveCallback);

void
ProcUIInitEx(ProcUISaveCallbackEx saveCallbackEx, void *arg);

uint32_t
ProcUIProcessMessages(BOOL unk1);

uint32_t
ProcUISubProcessMessages(BOOL unk1);

void
ProcUIRegisterCallback(ProcUICallbackType::Type type, ProcUICallback callback, void *param, uint32_t unk);

void
ProcUISetMEM1Storage(void *buffer, uint32_t size);
