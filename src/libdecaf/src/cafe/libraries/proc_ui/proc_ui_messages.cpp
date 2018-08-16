#include "proc_ui.h"
#include "proc_ui_messages.h"

namespace cafe::proc_ui
{

struct RegisteredCallback
{
   ProcUICallback callback;
   virt_ptr<void> param;
};

struct MessagesData
{
   be2_val<BOOL> sentAcquireMessage;
   be2_val<ProcUISaveCallback> saveCallback;
   be2_val<ProcUISaveCallbackEx> saveCallbackEx;
   be2_virt_ptr<void> saveCallbackExUserArg;
   be2_array<RegisteredCallback, ProcUICallbackType::Max> registeredCallbacks;
};

static virt_ptr<MessagesData>
sMessagesData = nullptr;

void
ProcUIInit(ProcUISaveCallback saveCallback)
{
   sMessagesData->saveCallback = saveCallback;
   sMessagesData->saveCallbackEx = nullptr;
   sMessagesData->saveCallbackExUserArg = nullptr;
}

void
ProcUIInitEx(ProcUISaveCallbackEx saveCallbackEx,
             virt_ptr<void> arg)
{
   sMessagesData->saveCallback = nullptr;
   sMessagesData->saveCallbackEx = saveCallbackEx;
   sMessagesData->saveCallbackExUserArg = arg;
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
                       virt_ptr<void> userArg,
                       uint32_t unk)
{
   if (type < sMessagesData->registeredCallbacks.size()) {
      sMessagesData->registeredCallbacks[type].callback = callback;
      sMessagesData->registeredCallbacks[type].param = userArg;
   }
}

void
ProcUISetMEM1Storage(virt_ptr<void> buffer,
                     uint32_t size)
{
   // TODO: ProcUISetMEM1Storage
}

void
Library::registerMessagesFunctions()
{
   RegisterFunctionExport(ProcUIInit);
   RegisterFunctionExport(ProcUIInitEx);
   RegisterFunctionExport(ProcUIProcessMessages);
   RegisterFunctionExport(ProcUISubProcessMessages);
   RegisterFunctionExport(ProcUIRegisterCallback);
   RegisterFunctionExport(ProcUISetMEM1Storage);

   RegisterDataInternal(sMessagesData);
}

} // namespace cafe::proc_ui
