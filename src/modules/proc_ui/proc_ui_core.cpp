#include "proc_ui.h"
#include "proc_ui_core.h"

static ProcUISaveCallback
gSaveCallback;

static ProcUISaveCallbackEx
gSaveCallbackEx;

static void *
gSaveCallbackExArg;

void
ProcUIInit(ProcUISaveCallback saveCallback)
{
   gSaveCallback = saveCallback;
   gSaveCallbackEx = nullptr;
   gSaveCallbackExArg = nullptr;
}

void
ProcUIInitEx(ProcUISaveCallbackEx saveCallbackEx, void *arg)
{
   gSaveCallback = nullptr;
   gSaveCallbackEx = saveCallbackEx;
   gSaveCallbackExArg = arg;
}

uint32_t
ProcUIProcessMessages(BOOL unk1)
{
   // TODO: ProcUIProcessMessages
   return 0;
}

uint32_t
ProcUISubProcessMessages(BOOL unk1)
{
   // TODO: ProcUISubProcessMessages
   return 0;
}

void
ProcUIRegisterCallback(ProcUICallbackType::Type type, ProcUICallback callback, void *param, uint32_t unk)
{
   // TODO: ProcUIRegisterCallback
}

void
ProcUISetMEM1Storage(void *buffer, uint32_t size)
{
   // TODO: ProcUISetMEM1Storage
}

void
ProcUI::registerCoreFunctions()
{
   RegisterKernelFunction(ProcUIInit);
   RegisterKernelFunction(ProcUIInitEx);
   RegisterKernelFunction(ProcUIProcessMessages);
   RegisterKernelFunction(ProcUISubProcessMessages);
   RegisterKernelFunction(ProcUIRegisterCallback);
   RegisterKernelFunction(ProcUISetMEM1Storage);
}
