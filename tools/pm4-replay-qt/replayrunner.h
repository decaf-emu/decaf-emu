#pragma once
#include "decaf.h"
#include "replay.h"
#include <QObject>

class ReplayRunner : public QObject
{
   Q_OBJECT

public:
   ReplayRunner(Decaf *decaf, std::shared_ptr<ReplayFile> replay) :
      mDecaf(decaf),
      mReplay(replay)
   {
      mRegisterStorage = reinterpret_cast<be_val<uint32_t> *>(mDecaf->heap()->alloc(0x10000 * 4, 0x100));
   }

public Q_SLOTS:
   void initialise();
   void runFrame();

Q_SIGNALS:
   void replayFinished();
   void frameFinished(unsigned int tv, unsigned int drc);

private:
   bool runPacket(ReplayIndex::Packet &packet);
   bool runCommand(ReplayIndex::Command &command);
   void runRegisterSnapshot(be_val<uint32_t> *registers, uint32_t count);
   void runGpu();

private:
   Decaf *mDecaf = nullptr;
   size_t mPacketIndex = 0;
   size_t mCommandIndex = 0;
   size_t mIndirectCommandIndex = 0;
   std::shared_ptr<ReplayFile> mReplay;
   be_val<uint32_t> *mRegisterStorage = nullptr;
   bool mRunning = true;
   ReplayPosition mPosition = { 0 , 0 };
};
