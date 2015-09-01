#pragma once
#include <string>
#include "systemtypes.h"

class DebugPacket;

class DebugNet
{
public:
   DebugNet();

   bool connect(const std::string& address, uint16_t port);

   void writePrelaunch();
   void writeBreakpointHit(uint32_t coreId, uint32_t userData);

protected:
   void writePacket(DebugPacket *pak);
   void handlePacket(DebugPacket *pak);
   void networkThread();

   void handleDisconnect();

   bool mConnected;
   void* mSocket;
   std::thread mNetworkThread;
};

extern DebugNet gDebugNet;
