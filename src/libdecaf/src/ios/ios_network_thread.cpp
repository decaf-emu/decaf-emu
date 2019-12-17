#include "ios_network_thread.h"

#include <algorithm>
#include <ares.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <uv.h>
#include <vector>

namespace ios::internal
{

static uv_loop_t sNetworkLoop = { };
static uv_async_t sAsyncPendingNetworkEvent = { };
static uv_timer_t sAresTimer = { };
static ares_channel sAresChannel = { };

static std::atomic<bool> sNetworkTaskThreadRunning { false };
static std::thread sNetworkTaskThread;
static std::mutex sPendingTasksMutex;
static std::vector<NetworkTask> sPendingTasks;

struct AresTask
{
   ares_socket_t socket;
   uv_poll_t poll;
};
static std::vector<std::unique_ptr<AresTask>> sAresTasks;

static void
uvAsyncCallback(uv_async_t *handle)
{
   std::vector<NetworkTask> tasks;
   sPendingTasksMutex.lock();
   sPendingTasks.swap(tasks);
   sPendingTasksMutex.unlock();

   for (const auto &task : tasks) {
      task();
   }

   if (!sNetworkTaskThreadRunning) {
      uv_stop(&sNetworkLoop);
   }
}

static void
aresTimerCallback(uv_timer_t *timer)
{
   ares_process_fd(sAresChannel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
}

static void
aresPollCallback(uv_poll_t *watcher, int status, int events)
{
   auto task = reinterpret_cast<AresTask *>(watcher->data);
   uv_timer_again(&sAresTimer);

   if (status < 0) {
     ares_process_fd(sAresChannel, task->socket, task->socket);
     return;
   }

   ares_process_fd(sAresChannel,
                   (events & UV_READABLE) ? task->socket : ARES_SOCKET_BAD,
                   (events & UV_WRITABLE) ? task->socket : ARES_SOCKET_BAD);
}

static void
aresSockstateCallback(void *data,
                      ares_socket_t sock,
                      int read,
                      int write)
{
   auto itr = std::find_if(sAresTasks.begin(), sAresTasks.end(),
                           [&](const auto &t) {
                              return t->socket == sock;
                           });
   if (read || write) {
      if (itr == sAresTasks.end()) {
         auto task = std::make_unique<AresTask>();
         uv_poll_init_socket(&sNetworkLoop, &task->poll, sock);
         task->poll.data = task.get();
         task->socket = sock;
         itr = sAresTasks.emplace(sAresTasks.end(), std::move(task));
      }

      uv_poll_start(&(*itr)->poll,
                    (read ? UV_READABLE : 0) | (write ? UV_WRITABLE : 0),
                    aresPollCallback);
   } else {
      if (itr != sAresTasks.end()) {
         uv_poll_stop(&(*itr)->poll);
         sAresTasks.erase(itr);
      }
   }
}

static void
networkTaskThread()
{
   ares_library_init(ARES_LIB_INIT_ALL);
   auto options = ares_options { 0 };
   options.flags = ARES_FLAG_NOCHECKRESP;
   options.sock_state_cb = &aresSockstateCallback;
   options.sock_state_cb_data = nullptr;
   ares_init_options(&sAresChannel, &options, ARES_OPT_FLAGS | ARES_OPT_SOCK_STATE_CB);

   uv_loop_init(&sNetworkLoop);

   uv_timer_init(&sNetworkLoop, &sAresTimer);
   uv_timer_start(&sAresTimer, aresTimerCallback, 1000, 1000);

   uv_async_init(&sNetworkLoop, &sAsyncPendingNetworkEvent, &uvAsyncCallback);
   uv_run(&sNetworkLoop, UV_RUN_DEFAULT);
   uv_loop_close(&sNetworkLoop);

   ares_destroy(sAresChannel);
   ares_library_cleanup();
}

void
startNetworkTaskThread()
{
   sNetworkTaskThreadRunning = true;
   sNetworkTaskThread = std::thread { networkTaskThread };
}

void
stopNetworkTaskThread()
{
   if (sNetworkTaskThreadRunning) {
      sNetworkTaskThreadRunning = false;
      uv_async_send(&sAsyncPendingNetworkEvent);
      sNetworkTaskThread.join();
   }
}

uv_loop_t *
networkUvLoop()
{
   return &sNetworkLoop;
}

ares_channel
networkAresChannel()
{
   return sAresChannel;
}

void
submitNetworkTask(NetworkTask task)
{
   sPendingTasksMutex.lock();
   sPendingTasks.emplace_back(std::move(task));
   sPendingTasksMutex.unlock();

   uv_async_send(&sAsyncPendingNetworkEvent);
}

} // ios::internal
