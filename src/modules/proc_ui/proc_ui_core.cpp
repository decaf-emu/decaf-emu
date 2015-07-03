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
   return 0;
}

uint32_t
ProcUISubProcessMessages(BOOL unk1)
{
   return 0;
}

void
ProcUI::registerCoreFunctions()
{
   RegisterKernelFunction(ProcUIInit);
   RegisterKernelFunction(ProcUIInitEx);
   RegisterKernelFunction(ProcUIProcessMessages);
   RegisterKernelFunction(ProcUISubProcessMessages);
}
