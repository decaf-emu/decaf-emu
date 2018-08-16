#pragma once
#include "nn_boss_taskid.h"
#include "nn_boss_titleid.h"

#include "cafe/libraries/nn_result.h"
#include "cafe/libraries/cafe_hle_library_typeinfo.h"

#include <libcpu/be2_struct.h>

/*
Unimplemented functions:
nn::boss::Task::CancelSync(nn::boss::CancelMode)
nn::boss::Task::CancelSync(unsigned int, nn::boss::CancelMode)
nn::boss::Task::Cancel(nn::boss::CancelMode)
nn::boss::Task::ClearTurnState(void)
nn::boss::Task::GetContentLength(unsigned int *) const
nn::boss::Task::GetExecCount(void) const
nn::boss::Task::GetHttpStatusCode(unsigned int *) const
nn::boss::Task::GetIntervalSec(void) const
nn::boss::Task::GetLifeTimeSec(void) const
nn::boss::Task::GetOptionResult(unsigned int, unsigned int *) const
nn::boss::Task::GetPriority(void) const
nn::boss::Task::GetProcessedLength(unsigned int *) const
nn::boss::Task::GetRemainingLifeTimeSec(void) const
nn::boss::Task::GetResult(unsigned int *) const
nn::boss::Task::GetRunningState(unsigned int *) const
nn::boss::Task::GetServiceStatus(void) const
nn::boss::Task::GetState(unsigned int *) const
nn::boss::Task::GetTaskID(void) const
nn::boss::Task::GetTitleID(void) const
nn::boss::Task::GetTurnState(unsigned int *) const
nn::boss::Task::GetUrl(char *, unsigned int) const
nn::boss::Task::IsFinished(void) const
nn::boss::Task::Reconfigure(nn::boss::TaskSetting const &)
nn::boss::Task::RegisterForImmediateRun(nn::boss::TaskSetting const &)
nn::boss::Task::Register(nn::boss::TaskSetting &)
nn::boss::Task::RestoreLifeTime(void)
nn::boss::Task::Run(bool)
nn::boss::Task::StartScheduling(bool)
nn::boss::Task::StopScheduling(void)
nn::boss::Task::Unregister(void)
nn::boss::Task::UpdateIntervalSec(unsigned int)
nn::boss::Task::UpdateLifeTimeSec(long long)
nn::boss::Task::Wait(nn::boss::TaskWaitState)
nn::boss::Task::Wait(unsigned int, nn::boss::TaskWaitState)
*/

namespace cafe::nn::boss
{

class Task
{
public:
   static virt_ptr<hle::VirtualTable> VirtualTable;
   static virt_ptr<hle::TypeDescriptor> TypeDescriptor;

public:
   Task();
   Task(virt_ptr<const char> taskId);
   Task(virt_ptr<const char> taskId,
        uint32_t accountId);
   Task(uint8_t slot,
        virt_ptr<const char> taskId);
   ~Task();

   nn::Result
   Initialize(virt_ptr<const char> taskId);

   nn::Result
   Initialize(virt_ptr<const char> taskId,
              uint32_t accountId);

   nn::Result
   Initialize(uint8_t slot,
              virt_ptr<const char> taskId);

   void
   Finalize();

   bool
   IsRegistered();

   uint32_t
   GetAccountID();

   void
   GetTaskID(virt_ptr<TaskID> id);

   void
   GetTitleID(virt_ptr<TitleID> id);

protected:
   be2_val<uint32_t> mAccountId;
   UNKNOWN(4);
   be2_struct<TaskID> mTaskId;
   be2_struct<TitleID> mTitleId;
   be2_virt_ptr<hle::VirtualTable> mVirtualTable;
   UNKNOWN(4);

protected:
   CHECK_MEMBER_OFFSET_START
      CHECK_OFFSET(Task, 0x00, mAccountId);
      CHECK_OFFSET(Task, 0x08, mTaskId);
      CHECK_OFFSET(Task, 0x10, mTitleId);
      CHECK_OFFSET(Task, 0x18, mVirtualTable);
   CHECK_MEMBER_OFFSET_END
};
CHECK_SIZE(Task, 0x20);

} // namespace cafe::nn::boss
