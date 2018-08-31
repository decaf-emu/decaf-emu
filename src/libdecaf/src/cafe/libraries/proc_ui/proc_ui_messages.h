#pragma once
#include "proc_ui_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::proc_ui
{

using ProcUISaveCallback = virt_func_ptr<
   void ()>;

using ProcUISaveCallbackEx = virt_func_ptr<
   uint32_t (virt_ptr<void> userArg)>;

using ProcUICallback = virt_func_ptr<
   uint32_t (virt_ptr<void> userArg)>;

void
ProcUIInit(ProcUISaveCallback saveCallback);

void
ProcUIInitEx(ProcUISaveCallbackEx saveCallbackEx,
             virt_ptr<void> userArg);

ProcUIStatus
ProcUIProcessMessages(BOOL block);

ProcUIStatus
ProcUISubProcessMessages(BOOL block);

void
ProcUIRegisterCallback(ProcUICallbackType type,
                       ProcUICallback callback,
                       virt_ptr<void> userArg,
                       uint32_t unk);

void
ProcUISetMEM1Storage(virt_ptr<void> buffer,
                     uint32_t size);

} // namespace cafe::proc_ui
