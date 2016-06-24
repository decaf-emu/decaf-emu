#include "coreinit.h"
#include "coreinit_fs.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_dir.h"
#include "coreinit_fs_file.h"
#include "coreinit_fs_path.h"
#include "coreinit_fs_stat.h"
#include "coreinit_internal_appio.h"
#include "coreinit_memheap.h"
#include "filesystem/filesystem.h"
#include <condition_variable>
#include <thread>
#include <mutex>
#include <queue>

namespace coreinit
{

static bool
sFsInitialised = false;

static uint32_t
sFsCoreId = 0;

void
FSInit()
{
   if (sFsInitialised) {
      return;
   }

   sFsCoreId = cpu::this_core::id();
   sFsInitialised = true;
}


void
FSShutdown()
{
}


void
FSInitCmdBlock(FSCmdBlock *block)
{
   memset(block, 0, sizeof(FSCmdBlock));
   block->priority = 16;
}


FSStatus
FSSetCmdPriority(FSCmdBlock *block,
                 FSPriority priority)
{
   block->priority = priority;
   return FSStatus::OK;
}

namespace internal
{

struct FSCmdBlockSortFn
{
   bool operator()(const FSCmdBlock *a, const FSCmdBlock *b) const
   {
      return a->priority < b->priority;
   }
};

static std::thread
sFsThread;

static std::atomic_bool
sFsThreadRunning;

static std::mutex
sFsQueueMutex;

static std::condition_variable
sFsQueueCond;

static std::priority_queue<FSCmdBlock*, std::vector<FSCmdBlock*>, FSCmdBlockSortFn>
sFsQueue;

static std::queue<FSCmdBlock*>
sFsDoneQueue;

void
handleFsDoneInterrupt()
{
   std::unique_lock<std::mutex> lock(sFsQueueMutex);
   while (!sFsDoneQueue.empty()) {
      auto item = sFsDoneQueue.front();
      sFsDoneQueue.pop();

      auto &ioMsg = item->result.ioMsg;
      ioMsg.message = &item->result;
      ioMsg.args[2] = AppIoEventType::FsAsyncCallback;

      auto destQueue = item->result.userParams.queue;
      if (destQueue) {
         OSSendMessage(destQueue, &ioMsg, OSMessageFlags::None);
      } else {
         internal::sendMessage(&ioMsg);
      }
   }
}

void fsThreadEntry()
{
   std::unique_lock<std::mutex> lock(sFsQueueMutex);

   while (sFsThreadRunning.load())
   {
      while (!sFsQueue.empty()) {
         auto item = sFsQueue.top();
         sFsQueue.pop();
         lock.unlock();

         item->result.status = item->func();

         lock.lock();

         sFsDoneQueue.push(item);
         cpu::interrupt(sFsCoreId, cpu::FS_DONE_INTERRUPT);
      }

      sFsQueueCond.wait(lock);
   }
}

void startFsThread()
{
   std::unique_lock<std::mutex> lock(sFsQueueMutex);
   sFsThreadRunning.store(true);
   sFsThread = std::thread(std::bind(&fsThreadEntry));
}

void shutdownFsThread()
{
   if (sFsThreadRunning.exchange(false)) {
      sFsQueueCond.notify_all();
      sFsThread.join();
   }
}

FSAsyncData *prepareSyncOp(FSClient *client, FSCmdBlock *block)
{
   OSInitMessageQueue(&block->syncQueue, block->syncQueueMsgs, 1);

   FSAsyncData *asyncData = &block->result.userParams;
   asyncData->callback = 0;
   asyncData->param = 0;
   asyncData->queue = &block->syncQueue;
   return asyncData;
}

FSStatus resolveSyncOp(FSClient *client, FSCmdBlock *block)
{
   OSMessage ioMsg;
   OSReceiveMessage(&block->syncQueue, &ioMsg, OSMessageFlags::Blocking);
   auto result = FSGetAsyncResult(&ioMsg);
   return result->status;
}

void queueFsWork(FSClient *client, FSCmdBlock *block, FSAsyncData *asyncData, std::function<FSStatus()> func)
{
   auto &asyncRes = block->result;
   asyncRes.userParams = *asyncData;
   asyncRes.client = client;
   asyncRes.block = block;

   block->func = func;
   std::unique_lock<std::mutex> lock(sFsQueueMutex);
   sFsQueue.push(block);
   sFsQueueCond.notify_all();
}

// We do not implement the following as I do not know the expected
//  behaviour when someone tries to cancel a synchronous operation.
bool
cancelFsWork(FSCmdBlock *cmd)
{
   return false;
}
void
cancelAllFsWork()
{
}

} // namespace internal

void
Module::registerFileSystemFunctions()
{
   RegisterKernelFunction(FSInit);
   RegisterKernelFunction(FSShutdown);
   RegisterKernelFunction(FSAddClient);
   RegisterKernelFunction(FSAddClientEx);
   RegisterKernelFunction(FSDelClient);
   RegisterKernelFunction(FSGetClientNum);
   RegisterKernelFunction(FSInitCmdBlock);
   RegisterKernelFunction(FSSetCmdPriority);
   RegisterKernelFunction(FSSetStateChangeNotification);
   RegisterKernelFunction(FSGetVolumeState);
   RegisterKernelFunction(FSGetLastErrorCodeForViewer);
   RegisterKernelFunction(FSGetAsyncResult);

   // coreinit_fs_path
   RegisterKernelFunction(FSGetCwd);
   RegisterKernelFunction(FSChangeDir);
   RegisterKernelFunction(FSChangeDirAsync);

   // coreinit_fs_stat
   RegisterKernelFunction(FSGetStat);
   RegisterKernelFunction(FSGetStatAsync);
   RegisterKernelFunction(FSGetStatFile);
   RegisterKernelFunction(FSGetStatFileAsync);

   // coreinit_fs_file
   RegisterKernelFunction(FSOpenFile);
   RegisterKernelFunction(FSOpenFileAsync);
   RegisterKernelFunction(FSCloseFile);
   RegisterKernelFunction(FSCloseFileAsync);
   RegisterKernelFunction(FSReadFile);
   RegisterKernelFunction(FSReadFileAsync);
   RegisterKernelFunction(FSReadFileWithPos);
   RegisterKernelFunction(FSReadFileWithPosAsync);
   RegisterKernelFunction(FSWriteFile);
   RegisterKernelFunction(FSWriteFileAsync);
   RegisterKernelFunction(FSWriteFileWithPos);
   RegisterKernelFunction(FSWriteFileWithPosAsync);
   RegisterKernelFunction(FSIsEof);
   RegisterKernelFunction(FSIsEofAsync);
   RegisterKernelFunction(FSGetPosFile);
   RegisterKernelFunction(FSGetPosFileAsync);
   RegisterKernelFunction(FSSetPosFile);
   RegisterKernelFunction(FSSetPosFileAsync);
   RegisterKernelFunction(FSTruncateFile);
   RegisterKernelFunction(FSTruncateFileAsync);

   // coreinit_fs_dir
   RegisterKernelFunction(FSOpenDir);
   RegisterKernelFunction(FSOpenDirAsync);
   RegisterKernelFunction(FSCloseDir);
   RegisterKernelFunction(FSCloseDirAsync);
   RegisterKernelFunction(FSMakeDir);
   RegisterKernelFunction(FSMakeDirAsync);
   RegisterKernelFunction(FSReadDir);
   RegisterKernelFunction(FSReadDirAsync);
   RegisterKernelFunction(FSRewindDir);
   RegisterKernelFunction(FSRewindDirAsync);
}

} // namespace coreinit
