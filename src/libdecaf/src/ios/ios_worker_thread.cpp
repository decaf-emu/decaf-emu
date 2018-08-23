#include "ios_worker_thread.h"
#include <atomic>
#include <functional>
#include <mutex>
#include <queue>

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
      sWorkerThreadConditionVariable.wait(lock);
      if (!sWorkerThreadRunning) {
         break;
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
   sWorkerThreadRunning = true;
   sWorkerThread = std::thread { iosWorkerThread };
}

void
joinWorkerThread()
{
   sWorkerThreadRunning = false;
   sWorkerThreadConditionVariable.notify_all();
   sWorkerThread.join();
}

void
submitWorkerTask(WorkerTask task)
{
   auto lock = std::unique_lock { sWorkerThreadMutex };
   sWorkerThreadTasks.push(std::move(task));
   sWorkerThreadConditionVariable.notify_all();
}

} // namespace ios::internal
