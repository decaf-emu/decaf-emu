#include "coreinit.h"
#include "coreinit_context.h"
#include "coreinit_core.h"
#include "coreinit_systeminfo.h"

#include "cafe/kernel/cafe_kernel_context.h"

namespace cafe::coreinit
{

struct StaticContextData
{
   be2_array<virt_ptr<OSContext>, 3> userContext;
};

static virt_ptr<StaticContextData> sContextData = nullptr;

void
OSInitContext(virt_ptr<OSContext> context,
              virt_addr nia,
              virt_addr stackBase)
{
   context->tag = OSContext::Tag;
   context->gpr.fill(0u);
   context->gpr[1] = stackBase;
   context->gpr[2] = internal::getSda2Base();
   context->gpr[13] = internal::getSdaBase();
   context->cr = 0u;
   context->lr = 0u;
   context->ctr = 0u;
   context->xer = 0u;
   context->nia = nia;
   context->cia = 0xFFFFFFFFu;
   context->gqr.fill(0u);
   context->spinLockCount = uint16_t { 0 };
   context->hostContext = nullptr;
   context->mmcr0 = 0u;
   context->mmcr1 = 0u;
   context->state = uint16_t { 0u };
   context->coretime.fill(0);
   context->starttime = 0ll;
   context->srr0 = 0u;
   context->srr1 = 0u;
   context->dsisr = 0u;
   context->dar = 0u;
   context->exceptionType = 0u;
}

void
OSSetCurrentContext(virt_ptr<OSContext> context)
{
   auto coreId = OSGetCoreId();
   auto prevContext = virt_ptr<OSContext> { nullptr };

   if (sContextData->userContext[coreId] &&
      (sContextData->userContext[coreId]->state & 8)) {
      prevContext = sContextData->userContext[coreId];
      kernel::copyContextFromCpu(prevContext);
   }

   sContextData->userContext[coreId] = context;
   kernel::copyContextToCpu(context);
}

void
OSSetCurrentFPUContext(uint32_t unk)
{
}

void
OSSetCurrentUserContext(virt_ptr<OSContext> context)
{
   sContextData->userContext[OSGetCoreId()] = context;
}

void
Library::registerContextSymbols()
{
   RegisterFunctionExport(OSInitContext);
   RegisterFunctionExport(OSSetCurrentContext);
   RegisterFunctionExport(OSSetCurrentFPUContext);
   RegisterFunctionExportName("__OSSetCurrentUserContext", OSSetCurrentUserContext);
   RegisterDataInternal(sContextData);
}

} // namespace cafe::coreinit
