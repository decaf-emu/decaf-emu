#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <regex>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#include "gdbstub.h"
#include "ppc.h"
#include "bitutils.h"
#include "memory.h"

#include "ovsocket/socket.h"
#include "ovsocket/networkthread.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "ovsocket.lib")

char *targetXML = "<?xml version=\"1.0\" ?>"
"<!DOCTYPE target SYSTEM \"gdb - target.dtd\">"
"<target>"
"  <architecture>powerpc:common</architecture>"
"  <feature name=\"org.gnu.gdb.power.core\">"
"     <reg name=\"r0\" bitsize = \"32\" type=\"uint32\"/>"
"     <reg name=\"r1\" bitsize = \"32\" type=\"uint32\"/>"
"     <reg name=\"r2\" bitsize = \"32\" type=\"uint32\"/>"
"     <reg name=\"r3\" bitsize = \"32\" type=\"uint32\"/>"
"     <reg name=\"r4\" bitsize = \"32\" type=\"uint32\"/>"
"     <reg name=\"r5\" bitsize = \"32\" type=\"uint32\"/>"
"     <reg name=\"r6\" bitsize = \"32\" type=\"uint32\"/>"
"     <reg name=\"r7\" bitsize = \"32\" type=\"uint32\"/>"
"     <reg name=\"r8\" bitsize = \"32\" type=\"uint32\"/>"
"     <reg name=\"r9\" bitsize = \"32\" type=\"uint32\"/>"
"     <reg name=\"r10\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r11\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r12\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r13\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r14\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r15\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r16\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r17\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r18\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r19\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r20\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r21\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r22\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r23\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r24\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r25\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r26\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r27\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r28\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r29\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r30\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"r31\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"pc\" bitsize=\"32\" type=\"code_ptr\" regnum=\"64\"/>"
"     <reg name=\"msr\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"cr\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"lr\" bitsize=\"32\" type=\"code_ptr\"/>"
"     <reg name=\"ctr\" bitsize=\"32\" type=\"uint32\"/>"
"     <reg name=\"xer\" bitsize=\"32\" type=\"uint32\"/>"
"  </feature>"
"<feature name=\"org.gnu.gdb.power.fpu\">"
"  <reg name=\"f0\" bitsize=\"64\" type=\"ieee_double\" regnum=\"32\"/>"
"  <reg name=\"f1\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f2\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f3\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f4\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f5\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f6\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f7\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f8\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f9\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f10\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f11\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f12\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f13\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f14\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f15\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f16\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f17\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f18\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f19\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f20\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f21\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f22\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f23\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f24\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f25\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f26\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f27\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f28\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f29\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f30\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"f31\" bitsize=\"64\" type=\"ieee_double\"/>"
"  <reg name=\"fpscr\" bitsize=\"32\" group=\"float\" regnum=\"70\"/>"
"</feature>"
"</target>";

/*
<feature name="OEA">
<reg name="sr0" bitsize="32" regnum="71"/>
<reg name="sr1" bitsize="32"/>
<reg name="sr2" bitsize="32"/>
<reg name="sr3" bitsize="32"/>
<reg name="sr4" bitsize="32"/>
<reg name="sr5" bitsize="32"/>
<reg name="sr6" bitsize="32"/>
<reg name="sr7" bitsize="32"/>
<reg name="sr8" bitsize="32"/>
<reg name="sr9" bitsize="32"/>
<reg name="sr10" bitsize="32"/>
<reg name="sr11" bitsize="32"/>
<reg name="sr12" bitsize="32"/>
<reg name="sr13" bitsize="32"/>
<reg name="sr14" bitsize="32"/>
<reg name="sr15" bitsize="32"/>

<reg name="pvr" bitsize="32"/>
<reg name="ibat0u" bitsize="32"/>
<reg name="ibat0l" bitsize="32"/>
<reg name="ibat1u" bitsize="32"/>
<reg name="ibat1l" bitsize="32"/>
<reg name="ibat2u" bitsize="32"/>
<reg name="ibat2l" bitsize="32"/>
<reg name="ibat3u" bitsize="32"/>
<reg name="ibat3l" bitsize="32"/>
<reg name="dbat0u" bitsize="32"/>
<reg name="dbat0l" bitsize="32"/>
<reg name="dbat1u" bitsize="32"/>
<reg name="dbat1l" bitsize="32"/>
<reg name="dbat2u" bitsize="32"/>
<reg name="dbat2l" bitsize="32"/>
<reg name="dbat3u" bitsize="32"/>
<reg name="dbat3l" bitsize="32"/>
<reg name="sdr1" bitsize="32"/>
<reg name="asr" bitsize="64"/>
<reg name="dar" bitsize="32"/>
<reg name="dsisr" bitsize="32"/>
<reg name="sprg0" bitsize="32"/>
<reg name="sprg1" bitsize="32"/>
<reg name="sprg2" bitsize="32"/>
<reg name="sprg3" bitsize="32"/>
<reg name="srr0" bitsize="32"/>
<reg name="srr1" bitsize="32"/>
<reg name="tbl" bitsize="32"/>
<reg name="tbu" bitsize="32"/>
<reg name="dec" bitsize="32"/>
<reg name="dabr" bitsize="32"/>
<reg name="ear" bitsize="32"/>
</feature>
<feature name="750">
<reg name="hid0" bitsize="32"/>
<reg name="hid1" bitsize="32"/>
<reg name="iabr" bitsize="32"/>
<reg name="dabr" bitsize="32"/>
<reg name="ummcr0" bitsize="32" regnum="124"/>
<reg name="upmc1" bitsize="32"/>
<reg name="upmc2" bitsize="32"/>
<reg name="usia" bitsize="32"/>
<reg name="ummcr1" bitsize="32"/>
<reg name="upmc3" bitsize="32"/>
<reg name="upmc4" bitsize="32"/>
<reg name="mmcr0" bitsize="32"/>
<reg name="pmc1" bitsize="32"/>
<reg name="pmc2" bitsize="32"/>
<reg name="sia" bitsize="32"/>
<reg name="mmcr1" bitsize="32"/>
<reg name="pmc3" bitsize="32"/>
<reg name="pmc4" bitsize="32"/>
<reg name="l2cr" bitsize="32"/>
<reg name="ictc" bitsize="32"/>
<reg name="thrm1" bitsize="32"/>
<reg name="thrm2" bitsize="32"/>
<reg name="thrm3" bitsize="32"/>
</feature>
*/


class GdbPacket
{
public:
   void addChar(char c)
   {
      mBuffer.push_back(c);
   }

   void addString(const std::string &str)
   {
      addString(str.data(), str.size());
   }

   void addString(const char *str)
   {
      addString(str, strlen(str));
   }

   void addString(const char *str, size_t size)
   {
      mBuffer.append(str, size);
   }

   void addEncodedString(const char *str)
   {
      addEncodedString(str, strlen(str));
   }

   void addEncodedString(const std::string &str)
   {
      addEncodedString(str.data(), str.size());
   }

   void addEncodedString(const char *str, size_t size)
   {
      for (auto i = 0u; i < size; ++i) {
         if (str[i] == '#' || str[i] == '$' || str[i] == '*' || str[i] == '}') {
            mBuffer += '}';
            mBuffer += str[i] ^ 0x20;
         } else {
            mBuffer += str[i];
         }
      }
   }

   void addHex8(uint8_t value)
   {
      char buffer[32];
      sprintf_s(buffer, 32, "%02X", value);
      addString(buffer);
   }

   void addHex16(uint16_t value)
   {
      char buffer[32];
      sprintf_s(buffer, 32, "%04X", value);
      addString(buffer);
   }

   void addHex32(uint32_t value)
   {
      char buffer[32];
      sprintf_s(buffer, 32, "%08X", value);
      addString(buffer);
   }

   void addHex64(uint64_t value)
   {
      char buffer[32];
      sprintf_s(buffer, 32, "%016llX", value);
      addString(buffer);
   }

   const std::string &data()
   {
      return mBuffer;
   }

private:
   std::string mBuffer;
};

class GdbServer
{
   static const size_t MAX_PACKET_LENGTH = 4096;

public:
   GdbServer(IDebugInterface *debugger) :
      mDebugger(debugger)
   {
   }

   bool start(const std::string &ip, const std::string &port)
   {
      mListenSocket = new ovs::Socket();

      if (!mListenSocket->listen(ip, port)) {
         return false;
      }

      using std::placeholders::_1;
      using std::placeholders::_2;
      using std::placeholders::_3;

      mListenSocket->addAcceptListener(std::bind(&GdbServer::onListenAccept, this, _1));
      mListenSocket->addDisconnectListener(std::bind(&GdbServer::onListenDisconnect, this, _1));
      mListenSocket->addErrorListener(std::bind(&GdbServer::onListenError, this, _1, _2));

      mThread = std::thread(&GdbServer::run, this);
      return true;
   }

   void join()
   {
      mThread.join();
   }

private:
   void run()
   {
      mNetThread.addSocket(mListenSocket);
      mNetThread.start();
   }

   void onListenAccept(ovs::Socket *socket)
   {
      using std::placeholders::_1;
      using std::placeholders::_2;
      using std::placeholders::_3;

      mClientSocket = socket->accept();
      mClientSocket->addErrorListener(std::bind(&GdbServer::onClientError, this, _1, _2));
      mClientSocket->addDisconnectListener(std::bind(&GdbServer::onClientDisconnect, this, _1));
      mClientSocket->addSendListener(std::bind(&GdbServer::onClientSend, this, _1, _2, _3));
      mClientSocket->addReceiveListener(std::bind(&GdbServer::onClientRead, this, _1, _2, _3));

      // Start receive
      mClientSocket->recvPartial(MAX_PACKET_LENGTH);
      mNetThread.addSocket(mClientSocket);
   }

   void onListenDisconnect(ovs::Socket *socket)
   {
   }

   void onListenError(ovs::Socket *socket, int code)
   {
   }

   void onClientDisconnect(ovs::Socket *socket)
   {
   }

   void onClientError(ovs::Socket *socket, int code)
   {
   }

   void onClientSend(ovs::Socket *socket, const char *buffer, std::size_t size)
   {
      std::cout << "send[" << size << "] " << std::string(buffer, size) << std::endl;
   }

   enum class ReadState
   {
      Idle,
      Command,
      Checksum1,
      Checksum2
   };

   void onClientRead(ovs::Socket *socket, const char *buffer, std::size_t size)
   {
      std::cout << "recv[" << size << "] " << std::string(buffer, size) << std::endl;

      for (auto i = 0; i < size; ++i) {
         unsigned char rsum, csum;

         // Check for response to previous packet
         if (mLastPacket.size()) {
            if (buffer[i] == '+' || buffer[i] == '$') {
               mLastPacket.clear();
            } else if (buffer[i] == '-') {
               socket->send(mLastPacket.data(), mLastPacket.size());
            } else {
               continue;
            }
         }

         // Read packet
         switch (mReadState) {
         case ReadState::Idle:
            if (buffer[i] == '$') {
               mReadState = ReadState::Command;
               mReceiveBuffer.clear();
               mChecksumBuffer.clear();
            }
            break;
         case ReadState::Command:
            if (buffer[i] == '#') {
               mReadState = ReadState::Checksum1;
            } else {
               mReceiveBuffer.push_back(buffer[i]);
            }
            break;
         case ReadState::Checksum1:
            mChecksumBuffer.push_back(buffer[i]);
            mReadState = ReadState::Checksum2;
            break;
         case ReadState::Checksum2:
            mChecksumBuffer.push_back(buffer[i]);
            rsum = std::stoul(mChecksumBuffer, NULL, 16);
            csum = 0;

            for (auto c : mReceiveBuffer) {
               csum += c;
            }

            if (rsum != csum) {
               socket->send("-", 1);
            } else {
               socket->send("+", 1);
               onReceivePacket(mReceiveBuffer);
            }

            mReadState = ReadState::Idle;
            break;
         }
      }

      // Continue receive
      mClientSocket->recvPartial(MAX_PACKET_LENGTH);
   }

   void onReceivePacket(const std::string &packet)
   {
      switch (packet[0]) {
      case '!':
         gdbEnableExtendedMode(packet);
         break;
      case 'c':
         sendPacket("S05");
         break;
      case '?':
         gdbHaltReason(packet);
         break;
      case 'm':
         gdbMemory(packet);
         break;
      case 'M':
         sendPacket("OK");
         break;
      case 'q':
         gdbQuery(packet);
         break;
      case 'p':
         gdbGetRegister(packet);
         break;
      case 'g':
         gdbGetAllRegisters(packet);
         break;
      case 'v':
         gdbVerbosePacket(packet);
         break;
      case 'z':
      case 'Z':
         gdbBreakpoint(packet);
         break;
      default:
         sendEmptyPacket();
      }
   }

   void gdbBreakpoint(const std::string &packet)
   {
      // 1,26afba4,1
      auto c1 = packet.find_first_of(',');
      auto c2 = packet.find_first_of(',', c1 + 1);

      auto types = packet.substr(1, c1 - 1);
      auto addrs = packet.substr(c1 + 1, c2 - c1 - 1);
      auto kinds = packet.substr(c2 + 1);

      auto type = std::stoull(types, NULL, 16);
      auto addr = std::stoull(addrs, NULL, 16);
      auto kind = std::stoull(kinds, NULL, 16);

      if (packet[0] == 'Z') {
         // Add
         mDebugger->addBreakpoint(addr);
      } else {
         // Remove
         mDebugger->removeBreakpoint(addr);
      }

      sendPacket("OK");
   }

   void gdbVerbosePacket(const std::string &packet)
   {
      if (packet.find("vCont?") == 0) {
         // List actions supported by vCont
         sendPacket("vCont;c;s;t");
      } else if (packet.find("vCont") == 0) {
         auto action = packet.substr(strlen("vCont;"));

         if (action[0] == 'c') {
            mDebugger->resume();
            sendPacket("S05");
         } else if (action[0] == 'c') {
            mDebugger->step();
            sendPacket("S05");
         } else if (action[0] == 't') {
            mDebugger->pause();
            sendPacket("S05");
         }
      } else {
         sendEmptyPacket();
      }
   }

   void gdbMemory(const std::string &packet)
   {
      auto sep = packet.find_first_of(',');
      auto addr = std::stoull(packet.substr(1, sep - 1), NULL, 16);
      auto length = std::stoull(packet.substr(sep + 1), NULL, 16);

      if (!gMemory.valid(addr) || !gMemory.valid(addr + length))
      {
         sendPacket("E14");
      } else {
         GdbPacket out;
         auto ptr = gMemory.translate(addr);

         for (auto i = 0; i < length; ++i) {
            out.addHex8(ptr[i]);
         }

         sendPacket(out.data());
      }
   }

   void gdbGetRegister(const std::string &packet)
   {
      GdbPacket out;
      auto state = mDebugger->getThreadState(0);
      auto id = std::stoull(packet.data() + 1, NULL, 16);

      if (id >= 0 && id < 32) {
         out.addHex32(state->gpr[id]);
      } else if (id >= 32 && id < 64) {
         out.addHex64(0);
      } else {
         switch (id) {
         case 64:
            out.addHex32(state->cia);
            break;
         case 65:
            out.addHex32(state->msr.value);
            break;
         case 66:
            out.addHex32(state->cr.value);
            break;
         case 67:
            out.addHex32(state->lr);
            break;
         case 68:
            out.addHex32(state->ctr);
            break;
         case 69:
            out.addHex32(state->xer.value);
            break;
         case 70:
            out.addHex32(state->fpscr.value);
            break;
         }
      }

      sendPacket(out.data());
   }

   void gdbGetAllRegisters(const std::string &packet)
   {
      GdbPacket out;
      auto state = mDebugger->getThreadState(0);

      // GPR
      for (auto i = 0; i < 32; ++i) {
         out.addHex32(state->gpr[i]);
      }

      // FPR
      for (auto i = 0; i < 32; ++i) {
         out.addHex64(bit_cast<uint64_t>(state->fpr[i]));
      }

      out.addHex32(state->cia);
      out.addHex32(state->msr.value);
      out.addHex32(state->cr.value);
      out.addHex32(state->lr);
      out.addHex32(state->ctr);
      out.addHex32(state->xer.value);
      out.addHex32(state->fpscr.value);
      sendPacket(out.data());
   }

   void gdbEnableExtendedMode(const std::string &packet)
   {
      sendPacket("OK");
   }

   void gdbHaltReason(const std::string &packet)
   {
      sendPacket("S05");
   }

   void gdbQuery(const std::string &packet)
   {
      if (packet.compare("qfThreadInfo") == 0) {
         // Begin thread list
         sendPacket("m01");
      } else if (packet.compare("qsThreadInfo") == 0) {
         // Continue thread list
         sendPacket("l");
      } else if (packet.compare("qC") == 0) {
         // Current thread ID
         sendPacket("QC01");
      } else if (packet.compare("qSupported") == 0) {
         sendPacket("PacketSize=1000;qXfer:features:read+");
      } else if (packet.find("qXfer:features:read:") == 0) {
         // Send our features shit
         auto info = packet.substr(strlen("qXfer:features:read:"));
         auto ofs = info.find_first_of(':');
         auto len = info.find_first_of(',');

         auto file = info.substr(0, ofs);
         auto offset = std::stoull(info.substr(ofs + 1, len - ofs - 1), NULL, 16);
         auto length = std::stoull(info.substr(len + 1), NULL, 16);

         if (info.find("target.xml") == 0) {
            GdbPacket packet;
            auto flen = strlen(targetXML);
            char *buf = new char[length * 2];

            if (offset + length < flen) {
               packet.addChar('m');
            } else {
               packet.addChar('l');
               length = flen - offset;
            }

            packet.addEncodedString(targetXML + offset, length);
            sendPacket(packet.data());
         }
      } else {
         sendEmptyPacket();
      }
   }

   void sendEmptyPacket()
   {
      sendPacket("");
   }

   void sendPacket(const std::string &data)
   {
      unsigned char csum = 0;

      for (auto c : data) {
         csum += c;
      }

      mLastPacket = data;
      mLastPacket.insert(mLastPacket.begin(), '$');
      mLastPacket.push_back('#');
      mLastPacket.resize(mLastPacket.size() + 2);
      sprintf(&mLastPacket[mLastPacket.size() - 2], "%02X", csum);
      mClientSocket->send(mLastPacket.data(), mLastPacket.size());
   }

private:
   ReadState mReadState = ReadState::Idle;
   std::string mLastPacket;
   std::string mReceiveBuffer;
   std::string mChecksumBuffer;

   std::thread mThread;
   ovs::NetworkThread mNetThread;
   ovs::Socket *mListenSocket;
   ovs::Socket *mClientSocket;

   IDebugInterface *mDebugger;
};


GdbServer *gServer;

void startGDBStub(IDebugInterface *debug)
{
   WSADATA wsaData;
   WSAStartup(MAKEWORD(2, 2), &wsaData);

   gServer = new GdbServer { debug };
   gServer->start("127.0.0.1", "23946");
}