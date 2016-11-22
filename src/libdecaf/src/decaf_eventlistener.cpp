#include "decaf_eventlistener.h"
#include <algorithm>
#include <vector>

namespace decaf
{

static std::vector<EventListener *>
sEventListeners;

void
addEventListener(EventListener *listener)
{
   sEventListeners.push_back(listener);
}

void
removeEventListener(EventListener *listener)
{
   sEventListeners.erase(std::remove(sEventListeners.begin(), sEventListeners.end(), listener));
}

namespace event
{

void
onGameLoaded(const decaf::GameInfo &info)
{
   for (auto listener : sEventListeners) {
      listener->onGameLoaded(info);
   }
}

} // namespace event

} // namespace decaf
