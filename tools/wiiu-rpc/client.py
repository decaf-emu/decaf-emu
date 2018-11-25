import socket, struct
import binascii
from functools import partial

CMD_DYNLOAD_ACQUIRE = 1000
CMD_DYNLOAD_RELEASE = 1001
CMD_DYNLOAD_FINDEXPORT = 1002

CMD_READ_MEMORY = 2000
CMD_WRITE_MEMORY = 2001
CMD_CALL_FUNCTION = 2002

CMD_IOS_OPEN = 3000
CMD_IOS_CLOSE = 3001
CMD_IOS_IOCTL = 3002
CMD_IOS_IOCTLV = 3003

ARG_TYPE_INT32 = 0
ARG_TYPE_INT64 = 1
ARG_TYPE_STRING = 2
ARG_TYPE_DATA_IN = 3
ARG_TYPE_DATA_OUT = 4
ARG_TYPE_DATA_IN_OUT = 5

class Packet:
   def __init__(self, command):
      self.command = command;
      self.data = ""
      self.readPos = 0

   def writeUint8(self, value):
      self.data += struct.pack(">B", value)

   def writeUint32(self, value):
      self.data += struct.pack(">I", value)

   def writeUint64(self, value):
      self.data += struct.pack(">Q", value)

   def writeInt32(self, value):
      self.data += struct.pack(">i", value)

   def writeInt64(self, value):
      self.data += struct.pack(">q", value)

   def writePointer(self, value):
      self.writeUint32(value)

   def writeString(self, value):
      self.writeUint32(len(value) + 1)
      self.data += value
      self.writeUint8(0)

   def writeBinary(self, value):
      self.writeUint32(len(value))
      self.data += value

   def readUint8(self):
      [value] = struct.unpack(">B", self.data[self.readPos : self.readPos + 1])
      self.readPos += 1
      return value

   def readInt32(self):
      [value] = struct.unpack(">i", self.data[self.readPos : self.readPos + 4])
      self.readPos += 4
      return value

   def readUint32(self):
      [value] = struct.unpack(">I", self.data[self.readPos : self.readPos + 4])
      self.readPos += 4
      return value

   def readPointer(self):
      return self.readUint32()

   def readInt64(self):
      [value] = struct.unpack(">q", self.data[self.readPos : self.readPos + 8])
      self.readPos += 8
      return value

   def readUint64(self):
      [value] = struct.unpack(">Q", self.data[self.readPos : self.readPos + 8])
      self.readPos += 8
      return value

   def readString(self):
      length = self.readUint32()
      value = self.data[self.readPos : self.readPos + length - 1]
      self.readUint8()
      self.readPos += length
      return value

   def readBinary(self):
      length = self.readUint32()
      value = self.data[self.readPos : self.readPos + length]
      self.readPos += length
      return value

class InOutData:
   def __init__(self, size):
      self.size = size
      self.data = ""

class OutData:
   def __init__(self, size):
      self.size = size
      self.data = ""

class Client:
   def __init__(self):
      self.fd = None

   def connect(self, ip, port):
      self.fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      self.fd.settimeout(5)
      self.fd.connect((ip, port))

   def disconnect(self):
      self.fd.close()
      self.fd = None

   def send(self, packet):
      header = struct.pack(">II", len(packet.data) + 8, packet.command)
      self.fd.sendall(header + packet.data)

   def recv(self):
      [length] = struct.unpack(">I", self.fd.recv(4))
      [command] = struct.unpack(">I", self.fd.recv(4))
      dataLength = length - 8

      packet = Packet(command)
      packet.readPos = 0
      packet.data = ""

      while len(packet.data) < dataLength:
         remaining = dataLength - len(packet.data)
         packet.data += self.fd.recv(remaining)

      return packet

   def OSDynLoad_Acquire(self, name):
      packet = Packet(CMD_DYNLOAD_ACQUIRE)
      packet.writeString(name)
      self.send(packet)

      reply = self.recv()
      result = reply.readInt32()
      handle = reply.readPointer()
      return [result, handle]

   def OSDynLoad_Release(self, handle):
      packet = Packet(CMD_DYNLOAD_RELEASE)
      packet.writePointer(handle)
      self.send(packet)
      self.recv()

   def OSDynLoad_FindExport(self, handle, isData, name):
      packet = Packet(CMD_DYNLOAD_FINDEXPORT)
      packet.writePointer(handle)
      packet.writeInt32(isData)
      packet.writeString(name)
      self.send(packet)

      reply = self.recv()
      result = reply.readInt32()
      handle = reply.readPointer()
      return [result, handle]

   def readMemory(self, addr, size):
      packet = Packet(CMD_READ_MEMORY)
      packet.writePointer(addr)
      packet.writeUint32(size)
      self.send(packet)

      reply = self.recv()
      data = reply.readBinary()
      return data

   def writeMemory(self, addr, data):
      packet = Packet(CMD_WRITE_MEMORY)
      packet.writePointer(addr)
      packet.writeBinary(data)
      self.send(packet)

      reply = self.recv()

   def callFunction(self, addr, *args):
      packet = Packet(CMD_CALL_FUNCTION)
      packet.writePointer(addr)
      packet.writeUint32(len(args))

      for arg in args:
         if type(arg) is int:
            packet.writeUint32(ARG_TYPE_INT32)
            packet.writeUint32(arg)
         elif type(arg) is long:
            packet.writeUint32(ARG_TYPE_INT64)
            packet.writeInt64(arg)
         elif type(arg) is str:
            packet.writeUint32(ARG_TYPE_STRING)
            packet.writeString(arg)
         elif isinstance(arg, OutData):
            packet.writeUint32(ARG_TYPE_DATA_OUT)
            packet.writeUint32(arg.size)
         elif isinstance(arg, InOutData):
            packet.writeUint32(ARG_TYPE_DATA_IN_OUT)
            packet.writeBinary(arg.data)

      self.send(packet)
      reply = self.recv()
      result = reply.readUint32()
      tmpDataOutNum = reply.readUint32()

      for i in range(0, tmpDataOutNum):
         argNum = reply.readUint32()
         data = reply.readBinary()
         args[argNum].data = data

      return result

   def createFunction(self, addr):
      return partial(self.callFunction, addr)

   def acquireLibrary(self, name):
      [result, handle] = self.OSDynLoad_Acquire(name)
      if result != 0:
         raise Exception("Module %s not found" % name)
      return handle

   def releaseLibrary(self, handle):
      self.OSDynLoad_Release(handle)

   def loadFunction(self, handle, name):
      [result, addr] = self.OSDynLoad_FindExport(handle, 0, name)
      if result != 0:
         raise Exception("Function %s not found" % name)

      return self.createFunction(addr)

   def iosIoctlv(self, handle, request, vecsIn, vecsOut):
      packet = Packet(CMD_IOS_IOCTLV)
      packet.writeUint32(handle)
      packet.writeUint32(request)
      packet.writeUint32(len(vecsIn))
      packet.writeUint32(len(vecsOut))

      for vec in vecsIn:
         packet.writeBinary(vec)

      for vec in vecsOut:
         packet.writeUint32(vec.size)

      self.send(packet)
      reply = self.recv()
      result = reply.readUint32()

      for vec in vecsOut:
         vec.data = reply.readBinary()

      return result

print "Connecting..."
client = Client()
client.connect("192.168.1.132", 1337)

# Example usage!
print "Connected"
coreinit = client.acquireLibrary("coreinit.rpl")

MCP_Open = client.loadFunction(coreinit, "MCP_Open")
MCP_TitleCount = client.loadFunction(coreinit, "MCP_TitleCount")
MCP_TitleList = client.loadFunction(coreinit, "MCP_TitleList")
MCP_Close = client.loadFunction(coreinit, "MCP_Close")

handle = MCP_Open()
titleCount = MCP_TitleCount(handle)

print "Title count: %d" % titleCount
if titleCount > 0:
   titleList = OutData(titleCount * 0x61)
   actualTitleCount = OutData(4)

   result = MCP_TitleList(handle, actualTitleCount, titleList, titleCount * 0x61)

   if result == 0:
      totalTitleCount = int(binascii.hexlify(actualTitleCount.data), 16)
      print "Total number of titles: %d" % totalTitleCount
      for i in range(0, totalTitleCount):
         print "Title " + str(i) + " ID: " + binascii.hexlify(titleList.data[i*0x61:i*0x61+8])

MCP_Close(handle)

client.releaseLibrary(coreinit)
client.disconnect()
