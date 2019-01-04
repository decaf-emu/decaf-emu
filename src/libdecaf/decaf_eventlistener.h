#pragma once
#include "decaf_game.h"

namespace decaf
{

enum class EventType
{
   GameLoaded,
};

class EventListener;

void
addEventListener(EventListener *listener);

void
removeEventListener(EventListener *listener);

class EventListener
{
public:
   virtual ~EventListener()
   {
      removeEventListener(this);
   }

   virtual void onGameLoaded(const GameInfo &info) { };
};

} // namespace decaf
