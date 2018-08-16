#include "debugger_server_gdb.h"
#include "debugger_threadutils.h"
#include "decaf_config.h"
#include "cafe/libraries/coreinit/coreinit_scheduler.h"
#include "cafe/libraries/coreinit/coreinit_thread.h"

#include <algorithm>
#include <common/platform_socket.h>
#include <common/strutils.h>
#include <common/log.h>
#include <fmt/format.h>
#include <libcpu/mmu.h>
#include <numeric>

namespace debugger
{

#include "debugger_server_gdb_xml.inl"

enum GdbCommand
{
   Ack = '+',
   Nack = '-',
   Break = 0x03,
   Start = '$',
   End = '#',
   Query = 'q',
   EnableExtendedMode = '!',
   GetHaltReason = '?',
   ReadRegister = 'p',
   ReadGeneralRegisters = 'g',
   ReadMemory = 'm',
   AddBreakpoint = 'Z',
   RemoveBreakpoint = 'z',
   VCommand = 'v',
   SetActiveThread = 'H',
};

enum RegisterID
{
   PC = 64,
   MSR = 65,
   CR = 66,
   LR = 67,
   CTR = 68,
   XER = 69,
};

enum BreakpointType
{
   Execute = 1,
};

class LogFormatter : public spdlog::formatter
{
public:
   void format(spdlog::details::log_msg& msg) override
   {
      auto tm_time = spdlog::details::os::localtime(spdlog::log_clock::to_time_t(msg.time));
      auto duration = msg.time.time_since_epoch();
      auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count() % 1000000;

      msg.formatted << '[' << fmt::pad(static_cast<unsigned int>(tm_time.tm_min), 2, '0') << ':'
         << fmt::pad(static_cast<unsigned int>(tm_time.tm_sec), 2, '0') << '.'
         << fmt::pad(static_cast<unsigned int>(micros), 6, '0');

      msg.formatted << ' ';
      msg.formatted << spdlog::level::to_str(msg.level);
      msg.formatted << ":gdb] ";

      msg.formatted << fmt::StringRef(msg.raw.data(), msg.raw.size());
      msg.formatted.write(spdlog::details::os::eol, spdlog::details::os::eol_size);
   }
};

GdbServer::GdbServer(DebuggerInterface *debugger,
                     ui::StateTracker *uiState) :
   mDebugger(debugger),
   mUiState(uiState)
{
}

bool GdbServer::start(int port)
{
   if (!mLog) {
      auto sinks = gLog->sinks();
      mLog = std::make_shared<spdlog::logger>("gdbstub", std::begin(sinks), std::end(sinks));
      mLog->set_level(gLog->level());
      mLog->set_formatter(std::make_shared<LogFormatter>());

      if (!decaf::config::log::async) {
         mLog->flush_on(spdlog::level::trace);
      }
   }

   mListenSocket = socket(PF_INET, SOCK_STREAM, 0);

   if (mListenSocket < 0) {
      mLog->error("Failed to create socket");
      return false;
   }

   // Set socket to SO_REUSEADDR so it can always bind on the same port
   auto sockOpt = 1;

   if (setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR,
                  reinterpret_cast<const char *>(&sockOpt),
                  sizeof(sockOpt)) < 0) {
      mLog->error("Failed to set SO_REUSEADDR on socket");
      closeServer();
      return false;
   }

   auto bindAddress = sockaddr_in { 0 };
   bindAddress.sin_family = AF_INET;
   bindAddress.sin_port = htons(port);
   bindAddress.sin_addr.s_addr = INADDR_ANY;

   if (bind(mListenSocket,
            reinterpret_cast<const sockaddr*>(&bindAddress),
            sizeof(sockaddr_in)) < 0) {
      mLog->error("Failed to bind on socket");
      closeServer();
      return false;
   }

   if (listen(mListenSocket, 1) < 0) {
      mLog->error("Failed to listen on socket");
      closeServer();
      return false;
   }

   platform::socketSetBlocking(mListenSocket, false);
   return true;
}

void GdbServer::closeServer()
{
   if (mListenSocket != InvalidSocket) {
      platform::socketClose(mListenSocket);
      mListenSocket = InvalidSocket;
   }
}

void GdbServer::closeClient()
{
   if (mClientSocket != InvalidSocket) {
      platform::socketClose(mClientSocket);
      mClientSocket = InvalidSocket;
   }
}

void GdbServer::handleBreak()
{
   mWasPaused = false;
   mDebugger->pause();
}

static std::string
encodeXml(const std::string &src)
{
   auto result = std::string { };
   result.reserve(src.size());

   for (auto i = 0u; i < src.size(); ++i) {
      auto c = src[i];

      if (c == '#' || c == '$' || c == '*' || c == '}') {
         result.push_back('}');
         result.push_back(c ^ 0x20);
      } else if (c == '\n' || c == '\r') {
         continue;
      } else {
         result.push_back(c);
      }
   }

   return result;
}

void GdbServer::handleQuery(const std::string &command)
{
   if (begins_with(command, "qSupported")) {
      auto features = std::string { };
      features += "PacketSize=1024;";
      features += "qXfer:features:read+;";
      // TODO: features += "qXfer:libraries:read+;";
      // TODO: features += "qXfer:memory-map:read+;";
      // TODO: features += "qXfer:threads:read+;";
      // TODO: features += "QStartNoAckMode+;";
      // TODO: features += "QThreadEvents+;";
      sendCommand(features);
   } else if (begins_with(command, "qfThreadInfo")) {
      fmt::MemoryWriter reply;
      cafe::coreinit::internal::lockScheduler();
      auto firstThread = cafe::coreinit::internal::getFirstActiveThread();

      if (firstThread) {
         reply.write("m");
      } else {
         reply.write("l");
      }

      for (auto thread = firstThread; thread; thread = thread->activeLink.next) {
         if (thread != firstThread) {
            reply.write(",");
         }

         reply.write("{:04X}", thread->id.value());
      }

      cafe::coreinit::internal::unlockScheduler();
      sendCommand(reply.str());
   } else if (begins_with(command, "qAttached")) {
      sendCommand("1");
   } else if (begins_with(command, "qsThreadInfo")) {
      sendCommand("l");
   } else if (begins_with(command, "qC")) {
      auto activeThread = mUiState->getActiveThread();

      if (activeThread) {
         auto reply = fmt::format("QC{:04X}", activeThread->id);
         sendCommand(reply);
      } else {
         sendCommand("");
      }
   } else if (begins_with(command, "qXfer:features:read:")) {
      std::vector<std::string> split;
      split_string(command, ':', split);

      auto xmlName = split[3];
      auto xml = std::string { };

      if (split[3] == "target.xml") {
         xml = encodeXml(sGdbTargetXML);
      } else if (split[3] == "power-core.xml") {
         xml = encodeXml(sGdbPowerCoreXml);
      }

      auto args = split[4];
      split.clear();
      split_string(args, ',', split);

      auto offset = std::stoul(split[0], 0, 16);
      auto size = std::stoul(split[1], 0, 16);
      auto reply = std::string { };

      if (offset + size < xml.size()) {
         reply += "m";
         reply += std::string { xml.data() + offset, size };
      } else {
         reply += "l";
         reply += std::string { xml.data() + offset, xml.size() - offset };
      }

      sendCommand(reply);
   } else if (begins_with(command, "qTStatus")) {
      // Trace not supported
      sendCommand("");
   } else {
      mLog->warn("Unknown query command {}", command);
      sendCommand("");
   }
}

void GdbServer::handleEnableExtendedMode(const std::string &command)
{
   sendCommand("");
}

void GdbServer::handleGetHaltReason(const std::string &command)
{
   sendCommand("T05");
}

void GdbServer::handleReadRegister(const std::string &command)
{
   auto id = std::stoul(command.substr(1), 0, 16);
   auto activeThread = mUiState->getActiveThread();
   auto coreRegs = getThreadCoreContext(mDebugger, activeThread);
   auto value = uint32_t { 0 };

   if (activeThread) {
      if (id < 32) {
         if (coreRegs) {
            value = coreRegs->gpr[id];
         } else {
            value = activeThread->context.gpr[id];
         }
      } else {
         switch (id) {
         case RegisterID::PC:
            value = getThreadNia(mDebugger, activeThread);
            break;
         case RegisterID::MSR:
            if (coreRegs) {
               value = coreRegs->msr.value;
            }
            break;
         case RegisterID::CR:
            if (coreRegs) {
               value = coreRegs->cr.value;
            } else {
               value = activeThread->context.cr;
            }
            break;
         case RegisterID::LR:
            if (coreRegs) {
               value = coreRegs->lr;
            } else {
               value = activeThread->context.lr;
            }
            break;
         case RegisterID::CTR:
            if (coreRegs) {
               value = coreRegs->ctr;
            } else {
               value = activeThread->context.ctr;
            }
            break;
         case RegisterID::XER:
            if (coreRegs) {
               value = coreRegs->xer.value;
            } else {
               value = activeThread->context.xer;
            }
            break;
         }
      }
   }

   sendCommand(fmt::format("{:08X}", value));
}

void GdbServer::handleReadGeneralRegisters(const std::string &command)
{
   fmt::MemoryWriter reply;
   auto activeThread = mUiState->getActiveThread();
   auto coreRegs = getThreadCoreContext(mDebugger, activeThread);

   for (auto i = 0; i < 32; ++i) {
      auto value = uint32_t { 0 };

      if (coreRegs) {
         value = coreRegs->gpr[i];
      } else {
         value = activeThread->context.gpr[i];
      }

      reply.write("{:08X}", value);
   }

   sendCommand(reply.str());
}

void GdbServer::handleReadMemory(const std::string &command)
{
   fmt::MemoryWriter reply;
   std::vector<std::string> split;
   split_string(command.data() + 1, ',', split);

   auto address = std::stoul(split[0], 0, 16);
   auto size = std::stoul(split[1], 0, 16);

   for (auto i = 0u; i < size; ++i) {
      auto value = uint8_t { 0 };

      if (cpu::isValidAddress(cpu::VirtualAddress { static_cast<uint32_t>(address + i) })) {
         value = mem::read<uint8_t>(address + i);
      }

      reply.write("{:02X}", value);
   }

   sendCommand(reply.str());
}

void GdbServer::handleAddBreakpoint(const std::string &command)
{
   std::vector<std::string> split;
   split_string(command.data() + 1, ',', split);

   auto type = std::stoul(split[0], 0, 16);
   auto address = std::stoul(split[1], 0, 16);
   auto length = std::stoul(split[2], 0, 16);

   if (type != BreakpointType::Execute) {
      sendCommand("E02");
   } else {
      mDebugger->addBreakpoint(address);
      sendCommand("OK");
   }
}

void GdbServer::handleRemoveBreakpoint(const std::string &command)
{
   std::vector<std::string> split;
   split_string(command.data() + 1, ',', split);

   auto type = std::stoul(split[0], 0, 16);
   auto address = std::stoul(split[1], 0, 16);
   auto length = std::stoul(split[2], 0, 16);

   if (type != BreakpointType::Execute) {
      sendCommand("E02");
   } else {
      mDebugger->removeBreakpoint(address);
      sendCommand("OK");
   }
}

void GdbServer::handleSetActiveThread(const std::string &command)
{
   auto type = command[1];
   auto id = std::stoi(command.data() + 2, 0, 16);
   auto thread = getThreadById(id);

   if (id == -1 || id == 0) {
      sendCommand("OK");
   } else if (thread) {
      mUiState->setActiveThread(thread);
      sendCommand("OK");
   } else {
      sendCommand("E22");
   }
}

void GdbServer::handleVContQuery(const std::vector<std::string> &command)
{
   sendCommand("vCont;c;C;s;S");
}

void GdbServer::handleVCont(const std::vector<std::string> &command)
{
   std::vector<std::string> split;
   split_string(command[1], ':', split);

   if (split[0] == "c") {
      mWasPaused = false;
      mDebugger->resume();
   } else if (split[0] == "s") {
      auto coreId = uint32_t { 1 };

      if (split.size()) {
         auto threadId = std::stoi(split[1], 0, 16);
         coreId = getCoreIdByThreadId(threadId);
      } else {
         coreId = getThreadCoreId(mUiState->getActiveThread());
      }

      if (coreId != ThreadNotRunning) {
         mDebugger->stepInto(coreId);
      } else {
         // Fuck it, default to core 1.
         mDebugger->stepInto(1);
      }
   }
}

void GdbServer::handleCommand(const std::string &command)
{
   switch (command[0]) {
   case GdbCommand::Query:
      handleQuery(command);
      break;
   case GdbCommand::EnableExtendedMode:
      handleEnableExtendedMode(command);
      break;
   case GdbCommand::GetHaltReason:
      handleGetHaltReason(command);
      break;
   case GdbCommand::ReadRegister:
      handleReadRegister(command);
      break;
   case GdbCommand::ReadGeneralRegisters:
      handleReadGeneralRegisters(command);
      break;
   case GdbCommand::ReadMemory:
      handleReadMemory(command);
      break;
   case GdbCommand::AddBreakpoint:
      handleAddBreakpoint(command);
      break;
   case GdbCommand::RemoveBreakpoint:
      handleRemoveBreakpoint(command);
      break;
   case GdbCommand::VCommand:
   {
      std::vector<std::string> vCommand;
      split_string(command, ';', vCommand);

      if (vCommand[0] == "vCont") {
         handleVCont(vCommand);
      } else if (vCommand[0] == "vCont?") {
         handleVContQuery(vCommand);
      } else if (vCommand[0] == "vMustReplyEmpty") {
         sendCommand("");
      } else {
         mLog->warn("Unknown vCommand {}", command);
         sendCommand("");
      }
      break;
   }
   case GdbCommand::SetActiveThread:
      handleSetActiveThread(command);
      break;
   default:
      mLog->warn("Unknown command {}", command);
      sendCommand("");
   }
}

template<class InputIt>
static uint8_t
calculateChecksum(InputIt first, InputIt last)
{
   return static_cast<uint8_t>(std::accumulate(first, last, 0, std::plus<uint8_t>()));
}

void GdbServer::sendAck()
{
   auto ack = char { GdbCommand::Ack };
   auto result = send(mClientSocket, &ack, 1, 0);

   if (result < 0) {
      mLog->error("Error sending ack");
   }
}

void GdbServer::sendNack()
{
   auto ack = char { GdbCommand::Nack };
   auto result = send(mClientSocket, &ack, 1, 0);

   if (result < 0) {
      mLog->error("Error sending nack");
   }
}

void GdbServer::sendCommand(const std::string &command)
{
   auto packet = std::string { };
   packet.push_back(GdbCommand::Start);
   packet.append(command);
   packet.push_back(GdbCommand::End);

   auto checksum = calculateChecksum(command.begin(), command.end());
   packet.append(fmt::format("{:02X}", checksum));

   auto bytesSent = 0;

   while (bytesSent < packet.size()) {
      auto result = send(mClientSocket,
                         packet.data() + bytesSent,
                         static_cast<int>(packet.size() - bytesSent),
                         0);

      if (result < 0) {
         mLog->error("Error sending command");
         break;
      }

      bytesSent += result;
   }

   mLastCommand = command;
}

void GdbServer::process()
{
   if (mListenSocket == InvalidSocket && mClientSocket == InvalidSocket) {
      return;
   }

   fd_set readfds;
   auto nfds = 0;
   auto tv = timeval { 0, 0 };
   FD_ZERO(&readfds);

   if (mListenSocket != InvalidSocket) {
      FD_SET(mListenSocket, &readfds);
      nfds = std::max(nfds, static_cast<int>(mListenSocket));
   }

   if (mClientSocket != InvalidSocket) {
      FD_SET(mClientSocket, &readfds);
      nfds = std::max(nfds, static_cast<int>(mClientSocket));
   }

   auto activity = select(nfds + 1, &readfds, NULL, NULL, &tv);

   if (mListenSocket != InvalidSocket && FD_ISSET(mListenSocket, &readfds)) {
      auto clientAddress = sockaddr_in { 0 };
      auto clientAddressLen = socklen_t { sizeof(sockaddr_in) };
      auto clientSocket = accept(mListenSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressLen);

      if (clientSocket < 0) {
         mLog->error("Failed to accept on socket");
      } else if (mClientSocket != InvalidSocket) {
         mLog->error("Rejecting connection because we already have a client connected.");
         platform::socketClose(clientSocket);
      } else {
         mClientSocket = clientSocket;
      }
   } else if (mClientSocket != InvalidSocket && FD_ISSET(mClientSocket, &readfds)) {
      while (mClientSocket != InvalidSocket) {
         char byte = 0;
         auto result = recv(mClientSocket, &byte, 1, 0);

         if (result < 0) {
            if (platform::socketWouldBlock(result)) {
               break;
            } else {
               mLog->debug("Client disconnected, recv returned {}", result);
               closeClient();
            }
         } else if (result == 0) {
            mLog->debug("Client disconnected gracefully");
            closeClient();
         } else {
            if (mReadState == ReadState::ReadStart) {
               if (byte == GdbCommand::Ack) {
                  continue;
               } else if (byte == GdbCommand::Nack) {
                  sendCommand(mLastCommand);
                  continue;
               } else if (byte == GdbCommand::Break) {
                  handleBreak();
               } else if (byte != GdbCommand::Start) {
                  mLog->error("Unexpected start of packet {}", byte);
                  closeClient();
               } else {
                  mReadState = ReadState::ReadCommand;
               }
            } else if (mReadState == ReadState::ReadCommand) {
               if (byte == GdbCommand::End) {
                  mReadState = ReadState::ReadChecksum;
               } else {
                  mReadBuffer.push_back(byte);
               }
            } else if (mReadState == ReadState::ReadChecksum) {
               mChecksumBuffer.push_back(byte);

               if (mChecksumBuffer.size() == 2) {
                  auto readChecksum = std::stoi(mChecksumBuffer, nullptr, 16);
                  auto calculatedChecksum = calculateChecksum(mReadBuffer.begin(), mReadBuffer.end());

                  if (readChecksum != calculatedChecksum) {
                     mLog->error("Unexpected command checksum {} != {}", readChecksum, calculatedChecksum);
                     sendNack();
                  } else {
                     sendAck();
                     handleCommand(mReadBuffer);
                  }

                  mReadState = ReadState::ReadStart;
                  mReadBuffer.clear();
                  mChecksumBuffer.clear();
               }
            }
         }
      }
   }

   // Check if we have become paused.
   auto activeThreadNia = getThreadNia(mDebugger, mUiState->getActiveThread());

   if (mWasPaused && activeThreadNia != 0 && mLastNia != activeThreadNia) {
      mWasPaused = false;
   }

   if (!mWasPaused && mDebugger->paused()) {
      if (mClientSocket != InvalidSocket) {
         sendCommand("T05");
      }

      mWasPaused = true;
      mLastNia = activeThreadNia;
   }
}

} // namespace debugger
