#include "coreinit.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_cmd.h"
#include "coreinit_fs_cmdblock.h"
#include "coreinit_fsa_shim.h"

namespace coreinit
{

namespace internal
{

static FSStatus
readFileWithPosAsync(FSClient *client,
                     FSCmdBlock *block,
                     uint8_t *buffer,
                     uint32_t size,
                     uint32_t count,
                     FSFilePosition pos,
                     FSFileHandle handle,
                     FSReadFlag readFlags,
                     FSErrorFlag errorMask,
                     FSAsyncData *asyncData);

} // namespace internal


/**
 * Change the client's working directory.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSChangeDir(FSClient *client,
            FSCmdBlock *block,
            const char *path,
            FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSChangeDirAsync(client, block, path, errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block,
                                              result, errorMask);
}


/**
 * Change the client's working directory (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSChangeDirAsync(FSClient *client,
                 FSCmdBlock *block,
                 const char *path,
                 FSErrorFlag errorMask,
                 FSAsyncData *asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   if (!path) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidPath);
      return FSStatus::FatalError;
   }

   auto error = internal::fsaShimPrepareRequestChangeDir(&blockBody->fsaShimBuffer,
                                                         clientBody->clientHandle,
                                                         path);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::fsCmdBlockFinishCmdFn);
   return FSStatus::OK;
}


/**
 * Close a file.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSCloseFile(FSClient *client,
            FSCmdBlock *block,
            FSFileHandle handle,
            FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);
   auto result = FSCloseFileAsync(client, block, handle, errorMask, &asyncData);
   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Close a file (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSCloseFileAsync(FSClient *client,
                 FSCmdBlock *block,
                 FSFileHandle handle,
                 FSErrorFlag errorMask,
                 const FSAsyncData *asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   auto error = internal::fsaShimPrepareRequestCloseFile(&blockBody->fsaShimBuffer,
                                                         clientBody->clientHandle,
                                                         handle);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::fsCmdBlockFinishCmdFn);
   return FSStatus::OK;
}


/**
 * Get the current working directory.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetCwd(FSClient *client,
         FSCmdBlock *block,
         char *returnedPath,
         uint32_t bytes,
         FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);
   auto result = FSGetCwdAsync(client, block, returnedPath, bytes,
                               errorMask, &asyncData);
   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Get the current working directory (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetCwdAsync(FSClient *client,
              FSCmdBlock *block,
              char *returnedPath,
              uint32_t bytes,
              FSErrorFlag errorMask,
              const FSAsyncData *asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   if (!returnedPath) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidBuffer);
      return FSStatus::FatalError;
   }

   if (bytes < FSMaxPathLength) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidParam);
      return FSStatus::FatalError;
   }

   blockBody->cmdData.getCwd.returnedPath = returnedPath;
   blockBody->cmdData.getCwd.bytes = bytes;

   auto error = internal::fsaShimPrepareRequestGetCwd(&blockBody->fsaShimBuffer,
                                                      clientBody->clientHandle);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::fsCmdBlockFinishCmdFn);
   return FSStatus::OK;
}


/**
 * Get the current read / write position of a file.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetPosFile(FSClient *client,
             FSCmdBlock *block,
             FSFileHandle handle,
             be_val<FSFilePosition> *returnedFpos,
             FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);
   auto result = FSGetPosFileAsync(client, block, handle, returnedFpos,
                                   errorMask, &asyncData);
   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Get the current read / write position of a file (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetPosFileAsync(FSClient *client,
                  FSCmdBlock *block,
                  FSFileHandle handle,
                  be_val<FSFilePosition> *returnedFpos,
                  FSErrorFlag errorMask,
                  FSAsyncData *asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   if (!returnedFpos) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidBuffer);
      return FSStatus::FatalError;
   }

   blockBody->cmdData.getPosFile.pos = returnedFpos;
   auto error = internal::fsaShimPrepareRequestGetPosFile(&blockBody->fsaShimBuffer,
                                                          clientBody->clientHandle,
                                                          handle);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::fsCmdBlockFinishCmdFn);
   return FSStatus::OK;
}


/**
 * Open a file.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 *
 * \retval FSStatus::NotFound
 * File not found.
 *
 * \retval FSStatus::NotFile
 * Used OpenFile on a non-file object such as a directory.
 *
 * \retval FSStatus::PermissionError
 * Did not have permission to open file in specified mode.
 */
FSStatus
FSOpenFile(FSClient *client,
           FSCmdBlock *block,
           const char *path,
           const char *mode,
           be_val<FSFileHandle> *fileHandle,
           FSErrorFlag errorMask)
{
   return FSOpenFileEx(client, block, path, mode,
                       0x660, 0, 0,
                       fileHandle, errorMask);
}


/**
 * Open a file (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 *
 * \retval FSStatus::NotFound
 * File not found.
 *
 * \retval FSStatus::NotFile
 * Used OpenFile on a non-file object such as a directory.
 *
 * \retval FSStatus::PermissionError
 * Did not have permission to open file in specified mode.
 */
FSStatus
FSOpenFileAsync(FSClient *client,
                FSCmdBlock *block,
                const char *path,
                const char *mode,
                be_val<FSFileHandle> *fileHandle,
                FSErrorFlag errorMask,
                const FSAsyncData *asyncData)
{
   return FSOpenFileExAsync(client, block, path, mode,
                            0x660, 0, 0,
                            fileHandle, errorMask, asyncData);
}


/**
 * Open a file.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 *
 * \retval FSStatus::NotFound
 * File not found.
 *
 * \retval FSStatus::NotFile
 * Used OpenFile on a non-file object such as a directory.
 *
 * \retval FSStatus::PermissionError
 * Did not have permission to open file in specified mode.
 */
FSStatus
FSOpenFileEx(FSClient *client,
             FSCmdBlock *block,
             const char *path,
             const char *mode,
             uint32_t unk1,
             uint32_t unk2,
             uint32_t unk3,
             be_val<FSFileHandle> *fileHandle,
             FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSOpenFileExAsync(client, block, path, mode,
                                   unk1, unk2, unk3,
                                   fileHandle, errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Open a file (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 *
 * \retval FSStatus::NotFound
 * File not found.
 *
 * \retval FSStatus::NotFile
 * Used OpenFile on a non-file object such as a directory.
 *
 * \retval FSStatus::PermissionError
 * Did not have permission to open file in specified mode.
 */
FSStatus
FSOpenFileExAsync(FSClient *client,
                  FSCmdBlock *block,
                  const char *path,
                  const char *mode,
                  uint32_t unk1,
                  uint32_t unk2,
                  uint32_t unk3,
                  be_val<FSFileHandle> *fileHandle,
                  FSErrorFlag errorMask,
                  const FSAsyncData *asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   if (!fileHandle) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidBuffer);
      return FSStatus::FatalError;
   }

   if (!path) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidPath);
      return FSStatus::FatalError;
   }

   if (!mode) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidParam);
      return FSStatus::FatalError;
   }

   blockBody->cmdData.openFile.handle = fileHandle;
   auto error = internal::fsaShimPrepareRequestOpenFile(&blockBody->fsaShimBuffer,
                                                        clientBody->clientHandle,
                                                        path, mode,
                                                        unk1, unk2, unk3);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::fsCmdBlockFinishCmdFn);
   return FSStatus::OK;
}


/**
 * Read a file.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSReadFile(FSClient *client,
           FSCmdBlock *block,
           uint8_t *buffer,
           uint32_t size,
           uint32_t count,
           FSFileHandle handle,
           FSReadFlag readFlags,
           FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSReadFileAsync(client, block, buffer, size, count, handle,
                                 readFlags, errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Read a file (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSReadFileAsync(FSClient *client,
                FSCmdBlock *block,
                uint8_t *buffer,
                uint32_t size,
                uint32_t count,
                FSFileHandle handle,
                FSReadFlag readFlags,
                FSErrorFlag errorMask,
                FSAsyncData *asyncData)
{
   return internal::readFileWithPosAsync(client, block, buffer, size, count,
                                         0, handle,
                                         static_cast<FSReadFlag>(readFlags & ~FSReadFlag::ReadWithPos),
                                         errorMask, asyncData);
}


/**
 * Read a file at a specific position.
 *
 * The files position will be set before reading.
 *
 * This is equivalent to:
 *    FSSetPosFile(file, pos)
 *    FSReadFile(file, ...)
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSReadFileWithPos(FSClient *client,
                  FSCmdBlock *block,
                  uint8_t *buffer,
                  uint32_t size,
                  uint32_t count,
                  FSFilePosition pos,
                  FSFileHandle handle,
                  FSReadFlag readFlags,
                  FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSReadFileWithPosAsync(client, block, buffer, size, count,
                                        handle, pos, readFlags, errorMask,
                                        &asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Read a file at a specific position (asynchronously).
 *
 * The files position will be set before reading.
 *
 * This is equivalent to:
 *    FSSetPosFile(file, pos)
 *    FSReadFile(file, ...)
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSReadFileWithPosAsync(FSClient *client,
                       FSCmdBlock *block,
                       uint8_t *buffer,
                       uint32_t size,
                       uint32_t count,
                       FSFilePosition pos,
                       FSFileHandle handle,
                       FSReadFlag readFlags,
                       FSErrorFlag errorMask,
                       FSAsyncData *asyncData)
{
   return internal::readFileWithPosAsync(client, block, buffer, size, count,
                                         0, handle,
                                         static_cast<FSReadFlag>(readFlags | FSReadFlag::ReadWithPos),
                                         errorMask, asyncData);
}


/**
 * Delete a file or directory.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSRemove(FSClient *client,
         FSCmdBlock *block,
         const char *path,
         FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSRemoveAsync(client, block, path, errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block,
                                              result, errorMask);
}


/**
 * Delete a file or directory (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSRemoveAsync(FSClient *client,
              FSCmdBlock *block,
              const char *path,
              FSErrorFlag errorMask,
              FSAsyncData *asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   if (!path) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidPath);
      return FSStatus::FatalError;
   }

   auto error = internal::fsaShimPrepareRequestRemove(&blockBody->fsaShimBuffer,
                                                      clientBody->clientHandle,
                                                      path);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::fsCmdBlockFinishCmdFn);
   return FSStatus::OK;
}


/**
 * Set the current read / write position for a file.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSSetPosFile(FSClient *client,
             FSCmdBlock *block,
             FSFileHandle handle,
             FSFilePosition pos,
             FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSSetPosFileAsync(client, block, handle, pos,
                                   errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block,
                                              result, errorMask);
}


/**
 * Set the current read / write position for a file (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSSetPosFileAsync(FSClient *client,
                  FSCmdBlock *block,
                  FSFileHandle handle,
                  FSFilePosition pos,
                  FSErrorFlag errorMask,
                  FSAsyncData *asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   auto error = internal::fsaShimPrepareRequestSetPosFile(&blockBody->fsaShimBuffer,
                                                          clientBody->clientHandle,
                                                          handle,
                                                          pos);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::fsCmdBlockFinishCmdFn);
   return FSStatus::OK;
}


namespace internal
{

FSStatus
readFileWithPosAsync(FSClient *client,
                     FSCmdBlock *block,
                     uint8_t *buffer,
                     uint32_t size,
                     uint32_t count,
                     FSFilePosition pos,
                     FSFileHandle handle,
                     FSReadFlag readFlags,
                     FSErrorFlag errorMask,
                     FSAsyncData *asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   // Ensure size * count is not > 32 bit.
   auto bytes = uint64_t { size } * uint64_t { count };

   if (bytes > 0xFFFFFFFFull) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidParam);
      return FSStatus::FatalError;
   }

   blockBody->cmdData.readFile.chunkSize = size;
   blockBody->cmdData.readFile.bytesRemaining = size * count;
   blockBody->cmdData.readFile.bytesRead = 0;

   // We only read up to FSMaxBytesPerRequest per request.
   if (size > FSMaxBytesPerRequest) {
      blockBody->cmdData.readFile.readSize = FSMaxBytesPerRequest;
   } else {
      blockBody->cmdData.readFile.readSize = size;
   }

   auto error = internal::fsaShimPrepareRequestReadFile(&blockBody->fsaShimBuffer,
                                                        clientBody->clientHandle,
                                                        buffer,
                                                        blockBody->cmdData.readFile.readSize,
                                                        1, pos, handle, readFlags);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::fsCmdBlockFinishReadCmdFn);
   return FSStatus::OK;
}

} // namespace internal

void
Module::registerFsCmdFunctions()
{
   RegisterKernelFunction(FSChangeDir);
   RegisterKernelFunction(FSChangeDirAsync);
   RegisterKernelFunction(FSCloseFile);
   RegisterKernelFunction(FSCloseFileAsync);
   RegisterKernelFunction(FSGetCwd);
   RegisterKernelFunction(FSGetCwdAsync);
   RegisterKernelFunction(FSGetPosFile);
   RegisterKernelFunction(FSGetPosFileAsync);
   RegisterKernelFunction(FSOpenFile);
   RegisterKernelFunction(FSOpenFileAsync);
   RegisterKernelFunction(FSOpenFileEx);
   RegisterKernelFunction(FSOpenFileExAsync);
   RegisterKernelFunction(FSReadFile);
   RegisterKernelFunction(FSReadFileAsync);
   RegisterKernelFunction(FSReadFileWithPos);
   RegisterKernelFunction(FSReadFileWithPosAsync);
   RegisterKernelFunction(FSRemove);
   RegisterKernelFunction(FSRemoveAsync);
   RegisterKernelFunction(FSSetPosFile);
   RegisterKernelFunction(FSSetPosFileAsync);
}

} // namespace coreinit
