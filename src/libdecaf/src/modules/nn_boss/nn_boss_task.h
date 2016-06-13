#pragma once
#include "modules/nn_result.h"
#include "modules/coreinit/coreinit_ghs_typeinfo.h"
#include "common/types.h"
#include "common/be_val.h"
#include "common/structsize.h"
#include "nn_boss_taskid.h"
#include "nn_boss_titleid.h"

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

namespace nn
{

namespace boss
{

class Task
{
public:
   static ghs::VirtualTableEntry *VirtualTable;
   static ghs::TypeDescriptor *TypeInfo;

public:
   Task();
   Task(const char *taskID);
   Task(const char *taskID, uint32_t accountID);
   Task(uint8_t slot, const char *taskID);
   ~Task();

   nn::Result
   Initialize(const char *taskID);

   nn::Result
   Initialize(const char *taskID, uint32_t accountID);

   nn::Result
   Initialize(uint8_t slot, const char *taskID);

   void
   Finalize();

   bool
   IsRegistered();

   uint32_t
   GetAccountID();

   void
   GetTaskID(TaskID *id);

   void
   GetTitleID(TitleID *id);

protected:
   be_val<uint32_t> mAccountID;
   UNKNOWN(4);
   TaskID mTaskID;
   TitleID mTitleID;
   be_ptr<ghs::VirtualTableEntry> mVirtualTable;
   UNKNOWN(4);

protected:
   CHECK_MEMBER_OFFSET_START
   CHECK_OFFSET(Task, 0x00, mAccountID);
   CHECK_OFFSET(Task, 0x08, mTaskID);
   CHECK_OFFSET(Task, 0x10, mTitleID);
   CHECK_OFFSET(Task, 0x18, mVirtualTable);
   CHECK_MEMBER_OFFSET_END
};
CHECK_SIZE(Task, 0x20);

} // namespace boss

} // namespace nn
