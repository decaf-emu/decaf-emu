#include <functional>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <sstream>
#include <iostream>
#include <thread>
#include "debugnet.h"
#include "debugmsg.h"
#include "debugger.h"
#include "system.h"
#include "processor.h"
#include "log.h"
#include "modules/coreinit/coreinit_thread.h"

DebugNet
gDebugNet;

struct DebugSymbolInfo {
   std::string name;
   uint32_t address;
   uint32_t type;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(name, address, type);
   }
};

struct DebugModuleInfo {
   std::string name;
   uint32_t entryPoint;
   std::vector<DebugSymbolInfo> symbols;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(name, entryPoint, symbols);
   }
};

struct DebugThreadInfo {
   std::string name;
   int32_t curCoreId;
   uint32_t attribs;
   uint32_t state;

   uint32_t cia;
   uint32_t gpr[32];
   uint32_t crf;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(name, curCoreId, attribs, state);
      ar(cia, gpr, crf);
   }
};

struct DebugPauseInfo {
   std::vector<DebugModuleInfo> modules;
   uint32_t userModuleIdx;
   std::vector<DebugThreadInfo> threads;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(modules);
      ar(userModuleIdx);
      ar(threads);
   }
};

void populateDebugPauseInfo(DebugPauseInfo& info) {
   auto &loadedModules = gLoader.getLoadedModules();
   int moduleIdx = 0;
   int userModuleIdx = -1;
   auto userModule = gSystem.getUserModule();
   for (auto &i : loadedModules) {
      auto &moduleName = i.first;
      auto &loadedModule = i.second;

      DebugModuleInfo tmod;
      tmod.name = moduleName;
      tmod.entryPoint = loadedModule->entryPoint();
      info.modules.push_back(tmod);

      if (loadedModule == userModule) {
         userModuleIdx = moduleIdx;
      }
      moduleIdx++;
   }
   info.userModuleIdx = static_cast<uint32_t>(userModuleIdx);

   auto &coreList = gProcessor.getCoreList();
   auto &fiberList = gProcessor.getFiberList();

   std::map<OSThread*, Fiber*> threads;
   for (auto &fiber : fiberList) {
      auto &thread = fiber->thread;
      auto titer = threads.find(thread);
      assert(titer == threads.end());

      threads.emplace(thread, fiber);
   }

   for (auto &i : threads) {
      auto &thread = i.first;
      auto &fiber = i.second;

      DebugThreadInfo tinfo;
      tinfo.name = thread->name;

      tinfo.curCoreId = -1;
      for (auto &core : coreList) {
         if (core->currentFiber) {
            if (thread == core->currentFiber->thread) {
               tinfo.curCoreId = core->id;
            }
         }
      }

      tinfo.attribs = thread->attr;
      tinfo.state = thread->state;

      tinfo.cia = fiber->state.cia;
      tinfo.crf = fiber->state.cr.value;
      memcpy(tinfo.gpr, fiber->state.gpr, sizeof(uint32_t) * 32);

      info.threads.push_back(tinfo);
   }
}

enum class DebugPacketType : uint16_t {
   Invalid = 0,
   PreLaunch = 1,
   BpHit = 2,
   Pause = 3,
   Resume = 4,
   AddBreakpoint = 5,
   RemoveBreakpoint = 6,
};

#pragma pack(push, 1)
struct DebugNetHeader {
   uint32_t size;
   DebugPacketType command;
   uint16_t reqId;
};
#pragma pack(pop)

class DebugPacket : public MessageClass<DebugPacketType> { };
template<DebugPacketType TypeId>
using DebugPacketBase = MessageClassBase<DebugPacket, TypeId>;

class DebugPacketBpHit : public DebugPacketBase<DebugPacketType::BpHit> {
public:
   uint32_t coreId;
   uint32_t userData;
   DebugPauseInfo info;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(coreId, userData, info);
   }

};

class DebugPacketPreLaunch : public DebugPacketBase<DebugPacketType::PreLaunch> {
public:
   DebugPauseInfo info;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(info);
   }

};

class DebugPacketPause : public DebugPacketBase<DebugPacketType::Pause> {
public:
   template <class Archive>
   void serialize(Archive &ar) { }

};

class DebugPacketResume : public DebugPacketBase<DebugPacketType::Resume> {
public:
   template <class Archive>
   void serialize(Archive &ar) { }

};

class DebugPacketBpAdd : public DebugPacketBase<DebugPacketType::AddBreakpoint> {
public:
   uint32_t address;
   uint32_t userData;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(address, userData);
   }

};

class DebugPacketBpRemove : public DebugPacketBase<DebugPacketType::RemoveBreakpoint> {
public:
   uint32_t address;

   template <class Archive>
   void serialize(Archive &ar) {
      ar(address);
   }

};

template <typename T>
int serializePacket2(std::vector<uint8_t> &data, DebugPacket *packet) {
   std::ostringstream str;
   cereal::BinaryOutputArchive archive(str);
   T &typedPacket = *(T*)packet;
   archive(typedPacket);
   std::string& payload = str.str();

   data.resize(sizeof(DebugNetHeader) + payload.size());
   auto &header = *reinterpret_cast<DebugNetHeader*>(data.data());
   header.size = (uint16_t)data.size();
   header.command = packet->type();
   std::copy(payload.begin(), payload.end(), data.begin() + sizeof(DebugNetHeader));
   return 0;
}

int serializePacket(std::vector<uint8_t> &data, DebugPacket *packet)
{
   if (packet->isA(DebugPacketType::BpHit)) {
      return serializePacket2<DebugPacketBpHit>(data, packet);
   } else if (packet->isA(DebugPacketType::PreLaunch)) {
      return serializePacket2<DebugPacketPreLaunch>(data, packet);
   } else if (packet->isA(DebugPacketType::Pause)) {
      return serializePacket2<DebugPacketPause>(data, packet);
   } else if (packet->isA(DebugPacketType::Resume)) {
      return serializePacket2<DebugPacketResume>(data, packet);
   } else {
      return -1;
   }
}

struct vectorwrapbuf : public std::basic_streambuf<char> {
   vectorwrapbuf(const uint8_t *start, const uint8_t *end) {
      this->setg((char*)start, (char*)start, (char*)end);
   }
};

template <typename T>
int deserializePacket2(const std::vector<uint8_t> &data, DebugPacket *&packet) {
   vectorwrapbuf databuf(data.data() + sizeof(DebugNetHeader), data.data() + data.size());
   std::istream stream(&databuf);
   cereal::BinaryInputArchive archive(stream);
   packet = new T();
   T &packetRef = *static_cast<T*>(packet);
   archive(packetRef);
   return 0;
}

int deserializePacket(const std::vector<uint8_t> &data, DebugPacket *&packet) {
   auto &header = *reinterpret_cast<const DebugNetHeader*>(data.data());
   assert(data.size() == header.size);

   if (header.command == DebugPacketType::BpHit) {
      return deserializePacket2<DebugPacketBpHit>(data, packet);
   } else if (header.command == DebugPacketType::PreLaunch) {
      return deserializePacket2<DebugPacketPreLaunch>(data, packet);
   } else if (header.command == DebugPacketType::Pause) {
      return deserializePacket2<DebugPacketPause>(data, packet);
   } else if (header.command == DebugPacketType::Resume) {
      return deserializePacket2<DebugPacketResume>(data, packet);
   } else {
      return -1;
   }
}

DebugNet::DebugNet()
{
   mConnected = false;
}

bool
DebugNet::connect(const std::string& address, uint16_t port)
{
   mConnected = false;

   WSADATA wsaData;
   WSAStartup(MAKEWORD(2, 2), &wsaData);

   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

   struct sockaddr_in server;
   server.sin_addr.s_addr = inet_addr(address.c_str());
   server.sin_family = AF_INET;
   server.sin_port = htons(port);

   int conres = ::connect(sock, (struct sockaddr *)&server, sizeof(server));
   if (conres < 0) {
      // Connect failure
      return false;
   }

   mConnected = true;
   mSocket = (void*)sock;
   mNetworkThread = std::thread(&DebugNet::networkThread, this);
   return true;
}

void
DebugNet::networkThread()
{
   SOCKET sock = (SOCKET)mSocket;
   std::vector<uint8_t> buffer;

   while (true) {
      buffer.resize(sizeof(DebugNetHeader));
      int recvd = recv(sock, (char*)&buffer[0], sizeof(DebugNetHeader), 0);
      if (recvd <= 0) {
         // Disconnected
         handleDisconnect();
         break;
      }
      assert(recvd == sizeof(DebugNetHeader));

      DebugNetHeader *header = (DebugNetHeader*)buffer.data();
      buffer.resize(header->size);

      recvd = recv(sock, 
         (char*)&buffer[sizeof(DebugNetHeader)],
         (int)buffer.size() - sizeof(DebugNetHeader), 
         0);
      if (recvd <= 0) {
         handleDisconnect();
         break;
      }
      assert(recvd == buffer.size() - sizeof(DebugNetHeader));

      DebugPacket *packet = nullptr;
      deserializePacket(buffer, packet);
      handlePacket(packet);
   }
}

void
DebugNet::handleDisconnect()
{
   mConnected = false;
   gDebugger.notify(new DebugMessageDebuggerDc());
}

void
DebugNet::handlePacket(DebugPacket *pak)
{
   gLog->debug("Handling packet {}", (int)pak->type());

   switch (pak->type()) {
   case DebugPacketType::AddBreakpoint: {
      auto *bpPak = static_cast<DebugPacketBpAdd*>(pak);
      gDebugger.addBreakpoint(bpPak->address, bpPak->userData);
      break;
   }
   case DebugPacketType::RemoveBreakpoint: {
      auto *bpPak = static_cast<DebugPacketBpRemove*>(pak);
      gDebugger.removeBreakpoint(bpPak->address);
      break;
   }
   case DebugPacketType::Pause: {
      gDebugger.pause();
      break;
   }
   case DebugPacketType::Resume: {
      gDebugger.resume();
      break;
   }
   }
}

void
DebugNet::writePacket(DebugPacket *pak)
{
   gLog->debug("Writing packet {}", (int)pak->type());

   std::vector<uint8_t> buffer;
   serializePacket(buffer, pak);

   SOCKET sock = (SOCKET)mSocket;
   send(sock, (char*)&buffer[0], (int)buffer.size(), 0);
}

void
DebugNet::writePrelaunch()
{
   auto pak = new DebugPacketPreLaunch();
   populateDebugPauseInfo(pak->info);
   writePacket(pak);
}

void
DebugNet::writeBreakpointHit(uint32_t coreId, uint32_t userData)
{
   auto pak = new DebugPacketBpHit();
   pak->coreId = coreId;
   pak->userData = userData;
   populateDebugPauseInfo(pak->info);
   writePacket(pak);
}
