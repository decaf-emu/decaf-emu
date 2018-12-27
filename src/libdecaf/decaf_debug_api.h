#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <vector>

namespace decaf::debug
{

using VirtualAddress = uint32_t;
using PhysicalAddress = uint32_t;

using CafeThreadHandle = VirtualAddress;

struct CafeMemorySegment
{
   //! Name of the memory segment.
   std::string name;

   //! Virtual Address of the start of the memory segment.
   VirtualAddress address;

   //! Size of the memory segment.
   uint32_t size;

   //! Alignment of the memory segment.
   uint32_t align;

   //! Segment has read permissions.
   bool read;

   //! Segment has write permissions.
   bool write;

   //! Segment has execute permissions.
   bool execute;
};

struct CafeModuleInfo
{
   //! Start address of text section.
   VirtualAddress textAddr = 0;

   //! Size of text.
   uint32_t textSize = 0;

   //! Start address of data section.
   VirtualAddress dataAddr = 0;

   //! Size of data.
   uint32_t dataSize = 0;
};

struct CafeThread
{
   enum ThreadAffinity
   {
      None = 0,
      Core0 = 1 << 0,
      Core1 = 1 << 1,
      Core2 = 1 << 2,
      Any = Core0 | Core1 | Core2,
   };

   enum ThreadState
   {
      //! Thread is inactive.
      Inactive = 0,

      //! Thread is ready to execute.
      Ready = 1 << 0,

      //! Thread is currently executing.
      Running = 1 << 1,

      //! Thread is blocked waiting for something e.g. a mutex.
      Waiting = 1 << 2,

      //! Thread is about to be terminated.
      Moribund = 1 << 3,
   };

   //! Virtual Address of the OSThread structure.
   CafeThreadHandle handle = 0;

   //! Assigned thread id.
   int id = -1;

   //! Name of the thread.
   std::string name;

   //! Current execution state.
   ThreadState state = ThreadState::Inactive;

   //! Thread core affinity.
   ThreadAffinity affinity = ThreadAffinity::None;

   //! Base thread priority.
   int basePriority = 0;

   //! Active thread priority.
   int priority = 0;

   //! The core this thread is currently running on, -1 if not running on a core.
   int coreId = -1;

   //! The amount of time this thread has been running.
   std::chrono::nanoseconds executionTime;

   //! The starting address of the stack.
   VirtualAddress stackStart = 0;

   //! The end address of the stack.
   VirtualAddress stackEnd = 0;

   //! Current execution address
   VirtualAddress cia = 0;

   //! Next execution address
   VirtualAddress nia = 0;

   //! Integer Registers
   std::array<uint32_t, 32> gpr;

   //! Floating-point Registers
   std::array<double, 32> fpr;

   //! Floating-point Registers - Paired Single 1
   std::array<double, 32> ps1;

   //! Condition Register
   uint32_t cr;

   //! XER Carry/Overflow register
   uint32_t xer;

   //! Link Register
   uint32_t lr;

   //! Count Register
   uint32_t ctr;

   //! Machine State Register
   uint32_t msr;
};

struct CpuContext
{
   //! Current execution address
   uint32_t cia;

   //! Next execution address
   uint32_t nia;

   //! Integer Registers
   std::array<uint32_t, 32> gpr;

   //! Floating-point Registers
   std::array<double, 32> fpr;

   //! Floating-point Registers - Paired Single 1
   std::array<double, 32> ps1;

   //! Condition Register
   uint32_t cr;

   //! XER Carry/Overflow register
   uint32_t xer;

   //! Link Register
   uint32_t lr;

   //! Count Register
   uint32_t ctr;

   //! Floating-Point Status and Control Register
   uint32_t fpscr;

   //! Processor Version Register
   uint32_t pvr;

   //! Machine State Register
   uint32_t msr;

   //! Segment Registers
   std::array<uint32_t, 16> sr;

   //! Graphics Quantization Registers
   std::array<uint32_t, 8> gqr;

   //! Data Address Register
   uint32_t dar;

   //! DSI Status Register
   uint32_t dsisr;

   //! Machine Status Save and Restore Register 0
   uint32_t srr0;
};

struct CafeVoice
{
   enum State
   {
      Stopped = 0,
      Playing = 1,
   };

   enum Format
   {
      ADPCM = 0,
      LPCM16 = 0x0A,
      LPCM8 = 0x19,
   };

   enum VoiceType
   {
      Default = 0,
      Streaming = 1,
   };

   //! The index of this voice.
   int index = -1;

   //! Current play state of the voice.
   State state = State::Stopped;

   //! Encoding format of the voice data.
   Format format;

   //! Voice type.
   VoiceType type;

   //! Address of voice data.
   VirtualAddress data;

   //! Current offset into voice data.
   int currentOffset;

   //! Loop offset of voice data.
   int loopOffset;

   //! End offset of voice data.
   int endOffset;

   //! Looping enabled.
   bool loopingEnabled;
};

//! Check if the debug API is ready to be used, this returns true once a game
//! .rpx has been loaded
bool ready();

// CafeOS
bool findClosestSymbol(VirtualAddress addr, uint32_t *outSymbolDistance,
                       char *symbolNameBuffer, uint32_t symbolNameBufferLength,
                       char *moduleNameBuffer, uint32_t moduleNameBufferLength);
bool getLoadedModuleInfo(CafeModuleInfo &info);
bool sampleCafeMemorySegments(std::vector<CafeMemorySegment> &segments);
bool sampleCafeRunningThread(int coreId, CafeThread &info);
bool sampleCafeThreads(std::vector<CafeThread> &threads);
bool sampleCafeVoices(std::vector<CafeVoice> &voiceInfos);

// Controller
bool pause();
bool resume();
bool isPaused();
int getPauseInitiatorCoreId();
const CpuContext *getPausedContext(int core);
bool stepInto(int core);
bool stepOver(int core);
bool hasBreakpoint(VirtualAddress address);
bool addBreakpoint(VirtualAddress address);
bool removeBreakpoint(VirtualAddress address);

// Memory
size_t readMemory(VirtualAddress address, void *dst, size_t size);
size_t writeMemory(VirtualAddress address, const void *src, size_t size);

} // namespace decaf::debug
