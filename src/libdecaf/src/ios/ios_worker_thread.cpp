#include "ios_worker_thread.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace ios::internal
{

using WorkerTask = std::function<void()>;

static std::thread
sWorkerThread;

static std::atomic<bool>
sWorkerThreadRunning { false };

static std::condition_variable
sWorkerThreadConditionVariable;

static std::mutex
sWorkerThreadMutex;

static std::queue<WorkerTask>
sWorkerThreadTasks;

static void
iosWorkerThread()
{
   while (sWorkerThreadRunning) {
      auto lock = std::unique_lock { sWorkerThreadMutex };

      if (sWorkerThreadTasks.empty()) {
         sWorkerThreadConditionVariable.wait(lock);
         if (!sWorkerThreadRunning) {
            break;
         }
      }

      auto task = std::move(sWorkerThreadTasks.front());
      sWorkerThreadTasks.pop();
      lock.unlock();

      task();
   }
}

void
startWorkerThread()
{
   if(!sWorkerThreadRunning) {
      sWorkerThreadRunning = true;
      sWorkerThread = std::thread { iosWorkerThread };
   }
}

void
stopWorkerThread()
{
   if(sWorkerThreadRunning) {
      sWorkerThreadRunning = false;
      sWorkerThreadConditionVariable.notify_all();
      sWorkerThread.join();
      sWorkerThreadTasks = {};
   }
}

void
submitWorkerTask(WorkerTask task)
{
   auto lock = std::unique_lock { sWorkerThreadMutex };
   sWorkerThreadTasks.push(std::move(task));
   sWorkerThreadConditionVariable.notify_all();
}

} // namespace ios::internal
