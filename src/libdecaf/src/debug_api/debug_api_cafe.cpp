#include "decaf_debug_api.h"

#include "cafe/loader/cafe_loader_entry.h"
#include "cafe/loader/cafe_loader_loaded_rpl.h"

#include "cafe/libraries/coreinit/coreinit_enum_string.h"
#include "cafe/libraries/coreinit/coreinit_scheduler.h"
#include "cafe/libraries/coreinit/coreinit_thread.h"
#include "cafe/libraries/sndcore2/sndcore2_enum.h"
#include "cafe/libraries/sndcore2/sndcore2_voice.h"
#include "cafe/loader/cafe_loader_entry.h"
#include "cafe/kernel/cafe_kernel_loader.h"

#include "debugger/debugger.h"

namespace decaf::debug
{

bool
findClosestSymbol(VirtualAddress addr,
                  uint32_t *outSymbolDistance,
                  char *symbolNameBuffer,
                  uint32_t symbolNameBufferLength,
                  char *moduleNameBuffer,
                  uint32_t moduleNameBufferLength)
{
   return cafe::kernel::internal::findClosestSymbol(virt_addr { addr },
                                                    outSymbolDistance,
                                                    symbolNameBuffer,
                                                    symbolNameBufferLength,
                                                    moduleNameBuffer,
                                                    moduleNameBufferLength) == 0;
}

bool
getLoadedModuleInfo(CafeModuleInfo &info)
{
   auto rpx = cafe::loader::getLoadedRpx();
   if (!rpx) {
      return false;
   }

   info.textAddr = static_cast<uint32_t>(rpx->textAddr);
   info.textSize = rpx->textSize;

   info.dataAddr = static_cast<uint32_t>(rpx->dataAddr);
   info.dataSize = rpx->textSize;
   return true;
}

bool
sampleCafeMemorySegments(std::vector<CafeMemorySegment> &segments)
{
   cafe::loader::lockLoader();
   for (auto rpl = cafe::loader::getLoadedRplLinkedList(); rpl; rpl = rpl->nextLoadedRpl) {
      if (!rpl->sectionAddressBuffer ||
          !rpl->sectionAddressBuffer ||
          !rpl->moduleNameBuffer ||
          !rpl->moduleNameLen ||
          !rpl->sectionAddressBuffer[rpl->elfHeader.shstrndx]) {
         continue;
      }

      auto rplName = std::string_view { rpl->moduleNameBuffer.get(),
                                        rpl->moduleNameLen };
      auto shStrTab =
         virt_cast<const char *>(rpl->sectionAddressBuffer[rpl->elfHeader.shstrndx])
         .get();

      for (auto i = 0u; i < rpl->elfHeader.shnum; ++i) {
         auto sectionHeader =
            virt_cast<cafe::loader::rpl::SectionHeader *>(
               virt_cast<virt_addr>(rpl->sectionHeaderBuffer) +
               (i * rpl->elfHeader.shentsize));

         if (rpl->sectionAddressBuffer[i] &&
             sectionHeader->size != 0 &&
             (sectionHeader->flags & cafe::loader::rpl::SHF_ALLOC)) {
            auto &segment = segments.emplace_back();
            segment.name = fmt::format("{}:{}", rplName, shStrTab + sectionHeader->name);
            segment.address = static_cast<uint32_t>(rpl->sectionAddressBuffer[i]);
            segment.size = sectionHeader->size;
            segment.align = sectionHeader->addralign;
            segment.read = sectionHeader->flags & cafe::loader::rpl::SHF_ALLOC;
            segment.write = sectionHeader->flags & cafe::loader::rpl::SHF_WRITE;
            segment.execute = sectionHeader->flags & cafe::loader::rpl::SHF_EXECINSTR;
         }
      }
   }
   cafe::loader::unlockLoader();
   return true;
}

static void
sampleThreadInfo(CafeThread &info, virt_ptr<cafe::coreinit::OSThread> thread)
{
   info.handle = static_cast<uint32_t>(virt_cast<virt_addr>(thread));
   info.id = thread->id;
   info.name = thread->name ? thread->name.get() : "";
   info.priority = thread->priority;
   info.basePriority = thread->basePriority;
   info.state = static_cast<CafeThread::ThreadState>(thread->state.value());
   info.affinity = static_cast<CafeThread::ThreadAffinity>(thread->attr & cafe::coreinit::OSThreadAttributes::AffinityAny);
   info.stackStart = static_cast<uint32_t>(virt_cast<virt_addr>(thread->stackStart));
   info.stackEnd = static_cast<uint32_t>(virt_cast<virt_addr>(thread->stackEnd));
   info.executionTime = std::chrono::nanoseconds { thread->coreTimeConsumedNs.value() };
}

static void
copyThreadRegisters(CafeThread &info, virt_ptr<cafe::coreinit::OSThread> thread)
{
   info.cia = thread->context.cia;
   info.nia = thread->context.nia;
   info.gpr = thread->context.gpr;
   info.fpr = thread->context.fpr;
   info.ps1 = thread->context.psf;
   info.cr = thread->context.cr;
   info.xer = thread->context.xer;
   info.lr = thread->context.lr;
   info.ctr = thread->context.ctr;
   info.msr = 0u;
}

static void
copyContextRegisters(CafeThread &info, const decaf::debug::CpuContext *context)
{
   info.cia = context->cia;
   info.nia = context->nia;
   info.gpr = context->gpr;
   info.fpr = context->fpr;
   info.ps1 = context->ps1;
   info.cr = context->cr;
   info.xer = context->xer;
   info.lr = context->lr;
   info.ctr = context->ctr;
   info.msr = context->msr;
}

bool
sampleCafeRunningThread(int coreId, CafeThread &info)
{
   if (!decaf::debug::isPaused()) {
      return false;
   }

   cafe::coreinit::internal::lockScheduler();
   auto thread = cafe::coreinit::internal::getCoreRunningThread(coreId);
   if (!thread) {
      cafe::coreinit::internal::unlockScheduler();
      return false;
   }

   sampleThreadInfo(info, thread);
   cafe::coreinit::internal::unlockScheduler();

   // Copy registers from paused context
   info.coreId = coreId;
   copyContextRegisters(info, decaf::debug::getPausedContext(coreId));
   info.executionTime += std::chrono::nanoseconds { cafe::coreinit::internal::getCoreThreadRunningTime(coreId) };
   return true;
}

bool
sampleCafeThreads(std::vector<CafeThread> &threads)
{
   auto paused = decaf::debug::isPaused();

   cafe::coreinit::internal::lockScheduler();
   virt_ptr<cafe::coreinit::OSThread> runningThreads[] = {
      cafe::coreinit::internal::getCoreRunningThread(0),
      cafe::coreinit::internal::getCoreRunningThread(1),
      cafe::coreinit::internal::getCoreRunningThread(2),
   };

   for (auto thread = cafe::coreinit::internal::getFirstActiveThread(); thread; thread = thread->activeLink.next) {
      auto &info = threads.emplace_back();
      sampleThreadInfo(info, thread);
      info.coreId = -1;

      if (paused) {
         for (auto i = 0u; i < 3; ++i) {
            if (thread == runningThreads[i]) {
               info.coreId = i;
               copyContextRegisters(info, decaf::debug::getPausedContext(i));
               info.executionTime += std::chrono::nanoseconds { cafe::coreinit::internal::getCoreThreadRunningTime(i) };
               break;
            }
         }
      }

      if (info.coreId == -1) {
         copyThreadRegisters(info, thread);
      }
   }

   cafe::coreinit::internal::unlockScheduler();
   return true;
}

bool
sampleCafeVoices(std::vector<CafeVoice> &voiceInfos)
{
   auto voices = cafe::sndcore2::internal::getAcquiredVoices();
   voiceInfos.resize(voices.size());

   for (auto i = 0u; i < voices.size(); ++i) {
      auto voice = voices[i];
      auto extras = cafe::sndcore2::internal::getVoiceExtras(voice->index);

      auto &voiceInfo = voiceInfos[i];
      voiceInfo.index = voice->index;
      voiceInfo.state = static_cast<CafeVoice::State>(voice->state);
      voiceInfo.format = static_cast<CafeVoice::Format>(voice->offsets.dataType);
      voiceInfo.type = static_cast<CafeVoice::VoiceType>(extras->type);
      voiceInfo.data = static_cast<VirtualAddress>(virt_cast<virt_addr>(voice->offsets.data));
      voiceInfo.currentOffset = static_cast<int>(voice->offsets.currentOffset);
      voiceInfo.loopOffset = static_cast<int>(voice->offsets.loopOffset);
      voiceInfo.endOffset = static_cast<int>(voice->offsets.endOffset);
      voiceInfo.loopingEnabled = (voice->offsets.loopingEnabled != cafe::sndcore2::AXVoiceLoop::Disabled);
   }

   return true;
}

} // namespace decaf::debug
