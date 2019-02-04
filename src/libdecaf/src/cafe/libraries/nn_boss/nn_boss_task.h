#pragma once
#include "nn_boss_taskid.h"
#include "nn_boss_titleid.h"

#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"
#include "nn/nn_result.h"

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

namespace cafe::nn_boss
{

struct Task
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;

   be2_val<uint32_t> accountId;
   UNKNOWN(4);
   be2_struct<TaskID> taskId;
   be2_struct<TitleID> titleId;
   be2_virt_ptr<ghs::VirtualTable> virtualTable;
   UNKNOWN(4);
};
CHECK_OFFSET(Task, 0x00, accountId);
CHECK_OFFSET(Task, 0x08, taskId);
CHECK_OFFSET(Task, 0x10, titleId);
CHECK_OFFSET(Task, 0x18, virtualTable);
CHECK_SIZE(Task, 0x20);

virt_ptr<Task>
Task_Constructor(virt_ptr<Task> self);

virt_ptr<Task>
Task_Constructor(virt_ptr<Task> self,
                 virt_ptr<const char> taskId);

virt_ptr<Task>
Task_Constructor(virt_ptr<Task> self,
                 virt_ptr<const char> taskId,
                 uint32_t accountId);

virt_ptr<Task>
Task_Constructor(virt_ptr<Task> self,
                 uint8_t slot,
                 virt_ptr<const char> taskId);

void
Task_Destructor(virt_ptr<Task> self,
                ghs::DestructorFlags flags);

nn::Result
Task_Initialize(virt_ptr<Task> self,
                virt_ptr<const char> taskId);

nn::Result
Task_Initialize(virt_ptr<Task> self,
                virt_ptr<const char> taskId,
                uint32_t accountId);

nn::Result
Task_Initialize(virt_ptr<Task> self,
                uint8_t slot,
                virt_ptr<const char> taskId);

void
Task_Finalize(virt_ptr<Task> self);

bool
Task_IsRegistered(virt_ptr<Task> self);

uint32_t
Task_GetAccountID(virt_ptr<Task> self);

void
Task_GetTaskID(virt_ptr<Task> self,
               virt_ptr<TaskID> id);

void
Task_GetTitleID(virt_ptr<Task> self,
                virt_ptr<TitleID> id);

} // namespace cafe::nn_boss
