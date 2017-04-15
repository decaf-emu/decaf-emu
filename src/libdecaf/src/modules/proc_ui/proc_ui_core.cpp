#include "proc_ui.h"
#include "proc_ui_core.h"

namespace proc_ui
{

static ProcUISaveCallback
sSaveCallback;

static ProcUISaveCallbackEx
sSaveCallbackEx;

static void *
sSaveCallbackExArg;

struct RegisteredCallback
{
   ProcUICallback callback;
   void *param;
};

std::array<RegisteredCallback, ProcUICallbackType::Max>
sRegisteredCallbacks;

static bool
sSentAcquireMessage = false;

void
ProcUIInit(ProcUISaveCallback saveCallback)
{
   sSaveCallback = saveCallback;
   sSaveCallbackEx = nullptr;
   sSaveCallbackExArg = nullptr;
}

void
ProcUIInitEx(ProcUISaveCallbackEx saveCallbackEx, void *arg)
{
   sSaveCallback = nullptr;
   sSaveCallbackEx = saveCallbackEx;
   sSaveCallbackExArg = arg;
}

ProcUIStatus
ProcUIProcessMessages(BOOL block)
{
   // TODO: ProcUIProcessMessages
   return ProcUIStatus::InForeground;
}

ProcUIStatus
ProcUISubProcessMessages(BOOL block)
{
   // TODO: ProcUISubProcessMessages
   return ProcUIStatus::InForeground;
}

void
ProcUIRegisterCallback(ProcUICallbackType type,
                       ProcUICallback callback,
                       void *param,
                       uint32_t unk)
{
   if (type < sRegisteredCallbacks.size()) {
      sRegisteredCallbacks[type].callback = callback;
      sRegisteredCallbacks[type].param = param;
   }
}

void
ProcUISetMEM1Storage(void *buffer, uint32_t size)
{
   // TODO: ProcUISetMEM1Storage
}

void
Module::registerCoreFunctions()
{
   RegisterKernelFunction(ProcUIInit);
   RegisterKernelFunction(ProcUIInitEx);
   RegisterKernelFunction(ProcUIProcessMessages);
   RegisterKernelFunction(ProcUISubProcessMessages);
   RegisterKernelFunction(ProcUIRegisterCallback);
   RegisterKernelFunction(ProcUISetMEM1Storage);
}

} // namespace proc_ui
