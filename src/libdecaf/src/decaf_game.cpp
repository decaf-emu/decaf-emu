#include "decaf_game.h"

namespace decaf
{

static GameInfo
sGameInfo;

const GameInfo &
getGameInfo()
{
   return sGameInfo;
}

} // namespace decaf
