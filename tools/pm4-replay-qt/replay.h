#pragma once
#include <array>
#include <libdecaf/decaf_pm4replay.h>
#include <libgpu/latte/latte_pm4.h>
#include <libgpu/latte/latte_pm4_commands.h>
#include <libgpu/latte/latte_pm4_reader.h>
#include <libgpu/latte/latte_pm4_writer.h>
#include <string>
#include <vector>
#include <memory>
#include <common/platform_memory.h>

using namespace latte::pm4;

struct ReplayPosition
{
   size_t packetIndex;
   size_t commandIndex;
};

struct ReplayIndex
{
   struct Frame
   {
      ReplayPosition pos;
   };

   struct Packet
   {
      decaf::pm4::CapturePacket::Type type;
      uint32_t size;
      uint8_t *data;
   };

   struct Command
   {
      latte::pm4::Header header;
      void *command;
   };

   std::vector<Frame> frames;
   std::vector<Packet> packets;
   std::vector<Command> commands;
};

struct ReplayFile
{
   ~ReplayFile()
   {
      if (view) {
         platform::unmapViewOfFile(view, size);
         view = nullptr;
      }

      if (handle != platform::InvalidMapFileHandle) {
         platform::closeMemoryMappedFile(handle);
         handle = platform::InvalidMapFileHandle;
      }

      size = 0;
   }

   platform::MapFileHandle handle = platform::InvalidMapFileHandle;
   uint8_t *view = nullptr;
   size_t size = 0;
   ReplayIndex index;
};

std::shared_ptr<ReplayFile>
openReplay(const std::string &path);

std::string
getCommandName(ReplayIndex::Command &command);

bool
buildReplayIndex(std::shared_ptr<ReplayFile> replay);
