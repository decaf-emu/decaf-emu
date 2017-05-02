#ifndef PACKET_H
#define PACKET_H

#include <wut_types.h>

typedef struct PacketReader
{
   uint32_t command;
   uint8_t *data;
   uint32_t dataLength;
   uint32_t pos;
} PacketReader;

typedef struct PacketWriter
{
   uint32_t command;
   uint32_t pos;
   void *buffer;
   uint32_t bufferSize;
} PacketWriter;

uint32_t
pakReadUint32(PacketReader *packet);

uint64_t
pakReadUint64(PacketReader *packet);

int32_t
pakReadInt32(PacketReader *packet);

int64_t
pakReadInt64(PacketReader *packet);

void *
pakReadPointer(PacketReader *packet);

const uint8_t *
pakReadData(PacketReader *packet, uint32_t *length);

const char *
pakReadString(PacketReader *packet);

void
pakWriteAlloc(PacketWriter *packet, uint32_t command);

void
pakWriteFree(PacketWriter *packet);

void
pakWriteUint32(PacketWriter *packet, uint32_t value);

void
pakWriteUint64(PacketWriter *packet, uint64_t value);

void
pakWriteInt32(PacketWriter *packet, int32_t value);

void
pakWriteInt64(PacketWriter *packet, int64_t value);

void
pakWritePointer(PacketWriter *packet, void *value);

void
pakWriteString(PacketWriter *packet, const char *str);

void
pakWriteData(PacketWriter *packet, const uint8_t *data, uint32_t length);

#endif // PACKET_H
