#include <cassert>
#include <cstdint>
#include <iostream>
#include <experimental/filesystem>
#include <fstream>
#include <memory>
#include <vector>
#include <ovsocket/socket.h>
#include <ovsocket/networkthread.h>
#include "hardware-test/hardwaretests.h"
#include "common/be_val.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "ovsocket.lib")

using namespace ovs;
using namespace std::placeholders;
namespace fs = std::experimental::filesystem;

// Deal with it.
static std::vector<hwtest::TestFile>
gTestSet;

#pragma pack(push, 1)
struct HWRegisterState
{
   be_val<uint32_t> xer;
   be_val<uint32_t> cr;
   be_val<uint32_t> ctr;
   be_val<uint32_t> _;
   be_val<uint32_t> r3;
   be_val<uint32_t> r4;
   be_val<uint32_t> r5;
   be_val<uint32_t> r6;
   be_val<uint64_t> fpscr;
   be_val<double> f1;
   be_val<double> f2;
   be_val<double> f3;
   be_val<double> f4;
};

struct PacketHeader
{
   enum Commands
   {
      Version = 1,
      ExecuteGeneralTest = 10,
      ExecutePairedTest = 20,
      TestsFinished = 50,
   };

   be_val<uint16_t> size;
   be_val<uint16_t> command;
};

struct VersionPacket : PacketHeader
{
   VersionPacket(uint32_t value)
   {
      size = sizeof(VersionPacket);
      command = PacketHeader::Version;
      version = value;
   }

   be_val<uint32_t> version;
};

struct ExecuteGeneralTestPacket : PacketHeader
{
   ExecuteGeneralTestPacket()
   {
      size = sizeof(ExecuteGeneralTestPacket);
      command = PacketHeader::ExecuteGeneralTest;
      memset(&state, 0, sizeof(HWRegisterState));
   }

   be_val<uint32_t> instr;
   HWRegisterState state;
};

struct TestFinishedPacket : PacketHeader
{
   TestFinishedPacket()
   {
      size = sizeof(VersionPacket);
      command = PacketHeader::TestsFinished;
   }
};
#pragma pack(pop)

class TestServer
{
   static const uint32_t Version = 1;

public:
   TestServer(Socket *socket) :
      mSocket(socket)
   {
      mSocket->addErrorListener(std::bind(&TestServer::onSocketError, this, _1, _2));
      mSocket->addDisconnectListener(std::bind(&TestServer::onSocketDisconnect, this, _1));
      mSocket->addReceiveListener(std::bind(&TestServer::onSocketReceive, this, _1, _2, _3));

      // Send version
      VersionPacket version { Version };
      mSocket->send(reinterpret_cast<const char *>(&version), sizeof(VersionPacket));

      // Read first packet
      mSocket->recvFill(sizeof(PacketHeader));

      // Initialise test iterators
      mTestFile = gTestSet.begin();
      mTestData = mTestFile->tests.begin();
   }

private:
   void saveTestFile()
   {
      // Save test result
      std::ofstream out("tests/cpu/wiiu/" + mTestFile->name, std::ofstream::out | std::ofstream::binary);
      cereal::BinaryOutputArchive ar(out);
      ar(*mTestFile);

      std::cout << "Wrote file tests/cpu/wiiu/" << mTestFile->name << std::endl;
   }

   void sendNextTest()
   {
      if (mTestData == mTestFile->tests.end()) {
         // Save current test file
         saveTestFile();

         // To the next test!
         ++mTestFile;

         if (mTestFile == gTestSet.end()) {
            std::cout << "Tests finished." << std::endl;

            // Notify client tests have finished
            TestFinishedPacket pak;
            mSocket->send(reinterpret_cast<const char *>(&pak), sizeof(TestFinishedPacket));
         } else {
            // Start next test file
            mTestData = mTestFile->tests.begin();
         }
      }

      // Copy test input
      ExecuteGeneralTestPacket packet;
      packet.instr = mTestData->instr.value;
      packet.state.xer = mTestData->input.xer.value;
      packet.state.cr = mTestData->input.cr.value;
      packet.state.ctr = mTestData->input.ctr;
      packet.state.r3 = mTestData->input.gpr[0];
      packet.state.r4 = mTestData->input.gpr[1];
      packet.state.r5 = mTestData->input.gpr[2];
      packet.state.r6 = mTestData->input.gpr[3];
      packet.state.fpscr = mTestData->input.fpscr.value;
      packet.state.f1 = mTestData->input.fr[0];
      packet.state.f2 = mTestData->input.fr[1];
      packet.state.f3 = mTestData->input.fr[2];
      packet.state.f4 = mTestData->input.fr[3];

      mSocket->send(reinterpret_cast<const char *>(&packet), sizeof(ExecuteGeneralTestPacket));
   }

   void handleTestResult(ExecuteGeneralTestPacket *result)
   {
      // Sanity check
      assert(mTestData->instr.value == result->instr.value());

      // Copy the output
      mTestData->output.xer.value = result->state.xer;
      mTestData->output.cr.value = result->state.cr;
      mTestData->output.ctr = result->state.ctr;
      mTestData->output.gpr[0] = result->state.r3;
      mTestData->output.gpr[1] = result->state.r4;
      mTestData->output.gpr[2] = result->state.r5;
      mTestData->output.gpr[3] = result->state.r6;
      mTestData->output.fpscr.value = static_cast<uint32_t>(result->state.fpscr);
      mTestData->output.fr[0] = result->state.f1;
      mTestData->output.fr[1] = result->state.f2;
      mTestData->output.fr[2] = result->state.f3;
      mTestData->output.fr[3] = result->state.f4;

      // Start next test
      mTestData++;
      sendNextTest();
   }

   void onReceivePacket(PacketHeader *packet)
   {
      VersionPacket *version;
      ExecuteGeneralTestPacket *result;

      switch (packet->command) {
      case PacketHeader::Version:
         // Receive version, begin tests
         version = reinterpret_cast<VersionPacket *>(packet);
         std::cout << "Server Version: " << Version << ", Client Version: " << version->version << std::endl;
         std::cout << "Running tests..." << std::endl;
         sendNextTest();
         break;
      case PacketHeader::ExecuteGeneralTest:
         // Receive test result
         result = reinterpret_cast<ExecuteGeneralTestPacket *>(packet);
         handleTestResult(result);
         break;
      }
   }

   void onSocketError(Socket *socket, int code)
   {
      assert(mSocket == socket);
      std::cout << "Socket error: " << code << std::endl;
   }

   void onSocketDisconnect(Socket *socket)
   {
      assert(mSocket == socket);
      std::cout << "Socket Disconnected" << std::endl;
   }

   void onSocketReceive(Socket *socket, const char *buffer, size_t size)
   {
      PacketHeader *header;
      assert(mSocket == socket);

      if (mCurrentPacket.size() == 0) {
         assert(size == sizeof(PacketHeader));

         // Copy packet to buffer
         mCurrentPacket.resize(size);
         header = reinterpret_cast<PacketHeader *>(mCurrentPacket.data());
         std::memcpy(header, buffer, size);

         // Read rest of packet
         auto read = header->size - size;
         socket->recvFill(read);
      } else {
         // Check we have read rest of packet
         header = reinterpret_cast<PacketHeader *>(mCurrentPacket.data());
         auto totalSize = size + sizeof(PacketHeader);
         assert(totalSize == header->size);

         // Read rest of packet
         mCurrentPacket.resize(totalSize);
         header = reinterpret_cast<PacketHeader *>(mCurrentPacket.data());
         std::memcpy(mCurrentPacket.data() + sizeof(PacketHeader), buffer, size);

         onReceivePacket(header);

         // Read next packet
         mCurrentPacket.clear();
         socket->recvFill(sizeof(PacketHeader));
      }
   }

private:
   Socket *mSocket;
   std::vector<char> mCurrentPacket;
   std::vector<hwtest::TestFile>::iterator mTestFile;
   std::vector<hwtest::TestData>::iterator mTestData;
};

static std::vector<std::unique_ptr<TestServer>>
gTestServers;

static void
loadTests()
{
   fs::create_directories("tests/cpu/wiiu");

   // Read all tests
   for (auto &entry : fs::directory_iterator("tests/cpu/input")) {
      std::ifstream file { entry.path().string(), std::ifstream::in | std::ifstream::binary };
      gTestSet.emplace_back();
      auto &test = gTestSet.back();
      test.name = entry.path().filename().string();

      // Parse cereal data
      cereal::BinaryInputArchive input(file);
      input(test);
   }
}

int main(int argc, char **argv)
{
   WSADATA wsaData;
   WSAStartup(MAKEWORD(2, 2), &wsaData);

   loadTests();

   NetworkThread thread;
   auto socket = new Socket();
   auto ip = "0.0.0.0";
   auto port = "8008";

   // On socket error
   socket->addErrorListener([](Socket *socket, int code) {
      std::cout << "Listen Socket Error: " << code << std::endl;
   });

   socket->addDisconnectListener([](Socket *socket) {
      std::cout << "Listen Socket Disconnected" << std::endl;
   });

   // On socket connected, accept pls
   socket->addAcceptListener([&thread](Socket *socket) {
      auto newSock = socket->accept();

      if (!newSock) {
         std::cout << "Failed to accept new connection" << std::endl;
         return;
      } else {
         std::cout << "New Connection Accepted" << std::endl;
      }

      gTestServers.emplace_back(new TestServer(socket));
      thread.addSocket(newSock);
   });

   // Start server
   if (!socket->listen(ip, port)) {
      std::cout << "Error starting connect!" << std::endl;
      return 0;
   }

   // Run network thread in main thread
   std::cout << "Listening on " << ip << ":" << port << std::endl;
   thread.addSocket(socket);
   thread.start();

   WSACleanup();
   return 0;
}
