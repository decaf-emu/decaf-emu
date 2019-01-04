#include "decaf_eventlistener.h"

#include <algorithm>
#include <mutex>
#include <vector>

namespace decaf
{

struct
{
   std::mutex mutex;
   std::vector<EventListener *> listeners;
} sEventListenerData;

void
addEventListener(EventListener *listener)
{
   std::unique_lock<std::mutex> lock { sEventListenerData.mutex };
   sEventListenerData.listeners.push_back(listener);
}

void
removeEventListener(EventListener *listener)
{
   std::unique_lock<std::mutex> lock { sEventListenerData.mutex };
   sEventListenerData.listeners.erase(
      std::remove(sEventListenerData.listeners.begin(),
                  sEventListenerData.listeners.end(),
                  listener));
}

namespace event
{

void
onGameLoaded(const decaf::GameInfo &info)
{
   sEventListenerData.mutex.lock();
   auto listeners = sEventListenerData.listeners;
   sEventListenerData.mutex.unlock();

   for (auto listener : listeners) {
      listener->onGameLoaded(info);
   }
}

} // namespace event

} // namespace decaf
