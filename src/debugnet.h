#pragma once
#include <cstdint>
#include <string>

class DebugPacket;

class DebugNet
{
public:
   DebugNet();

   void setTarget(const std::string& address, uint16_t port);
   bool connect();

   void writePrelaunch();
   void writeBreakpointHit(uint32_t coreId, uint32_t userData);
   void writeCoreStepped(uint32_t coreId);
   void writePaused();

protected:
   void writePacket(DebugPacket *pak);
   void handlePacket(DebugPacket *pak);
   void networkThread();

   void handleDisconnect();

   bool mConnected;
   void* mSocket;
   std::string mAddress;
   uint16_t mPort;
   std::thread *mNetworkThread;
};

extern DebugNet gDebugNet;
