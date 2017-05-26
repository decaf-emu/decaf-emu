#include <hle_test.h>
#include <coreinit/filesystem.h>

int main(int argc, char **argv)
{
   FSClient client;
   FSCmdBlock cmdBlock;
   FSFileHandle fh;
   FSStatus status;
   char buffer[6];
   size_t readSize;

   test_report("Initialise filesystem");
   FSInit();
   FSAddClient(&client, 0);
   FSInitCmdBlock(&cmdBlock);

   test_report("Open file");
   status = FSOpenFile(&client, &cmdBlock, "/vol/content/short_text.txt", "r", &fh, -1);
   test_report("FSOpenFile status: %d fh: %u", status, fh);
   test_eq(status, 0);

   test_report("Read file");
   readSize = sizeof(buffer) - 1;
   status = FSReadFile(&client, &cmdBlock, buffer, 1, readSize, fh, 0, -1);
   buffer[readSize] = 0;
   test_report("FSReadFile status: %d, buffer: %s", status, buffer);
   test_eq(status, readSize);

   test_report("Close file");
   status = FSCloseFile(&client, &cmdBlock, fh, 0);
   test_report("FSCloseFile status: %d", status);
   test_eq(status, 0);

   test_report("Shutdown filesystem");
   FSDelClient(&client, 0);
   FSShutdown();
   return 0;
}
