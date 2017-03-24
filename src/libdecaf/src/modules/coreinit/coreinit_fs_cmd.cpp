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
                     const FSAsyncData *asyncData);

static FSStatus
writeFileWithPosAsync(FSClient *client,
                      FSCmdBlock *block,
                      const uint8_t *buffer,
                      uint32_t size,
                      uint32_t count,
                      FSFilePosition pos,
                      FSFileHandle handle,
                      FSWriteFlag writeFlags,
                      FSErrorFlag errorMask,
                      const FSAsyncData *asyncData);

static FSStatus
getInfoByQueryAsync(FSClient *client,
                    FSCmdBlock *block,
                    const char *path,
                    FSQueryInfoType type,
                    void *out,
                    FSErrorFlag errorMask,
                    const FSAsyncData *asyncData);

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
                 const FSAsyncData *asyncData)
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
 * Close a directory.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSCloseDir(FSClient *client,
           FSCmdBlock *block,
           FSDirHandle handle,
           FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSCloseDirAsync(client, block, handle, errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Close a directory (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSCloseDirAsync(FSClient *client,
                FSCmdBlock *block,
                FSDirHandle handle,
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

   auto error = internal::fsaShimPrepareRequestCloseDir(&blockBody->fsaShimBuffer,
                                                        clientBody->clientHandle,
                                                        handle);

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
 * Flush the contents of a file to disk.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSFlushFile(FSClient *client,
            FSCmdBlock *block,
            FSFileHandle handle,
            FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSFlushFileAsync(client, block, handle, errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Flush the contents of a file to disk (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSFlushFileAsync(FSClient *client,
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

   auto error = internal::fsaShimPrepareRequestFlushFile(&blockBody->fsaShimBuffer,
                                                         clientBody->clientHandle,
                                                         handle);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::fsCmdBlockFinishCmdFn);
   return FSStatus::OK;
}


/**
 * I don't know what this does :).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSFlushQuota(FSClient *client,
             FSCmdBlock *block,
             const char *path,
             FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSFlushQuotaAsync(client, block, path, errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * I don't know what this does :) (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSFlushQuotaAsync(FSClient *client,
                  FSCmdBlock *block,
                  const char *path,
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

   if (!path) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidBuffer);
      return FSStatus::FatalError;
   }

   auto error = internal::fsaShimPrepareRequestFlushQuota(&blockBody->fsaShimBuffer,
                                                          clientBody->clientHandle,
                                                          path);

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
 * Get free space for entry at path.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 *
 * \retval FSStatus::NotFound
 * Entry not found.
 */
FSStatus
FSGetFreeSpaceSize(FSClient *client,
                   FSCmdBlock *block,
                   const char *path,
                   be_val<uint64_t> *returnedFreeSize,
                   FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSGetFreeSpaceSizeAsync(client, block, path, returnedFreeSize,
                                         errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Get free space for entry at path.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetFreeSpaceSizeAsync(FSClient *client,
                        FSCmdBlock *block,
                        const char *path,
                        be_val<uint64_t> *returnedFreeSize,
                        FSErrorFlag errorMask,
                        const FSAsyncData *asyncData)
{
   return internal::getInfoByQueryAsync(client, block, path,
                                        FSQueryInfoType::FreeSpaceSize,
                                        returnedFreeSize,
                                        errorMask, asyncData);
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
                  const FSAsyncData *asyncData)
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
 * Get statistics about a filesystem entry.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 *
 * \retval FSStatus::NotFound
 * Entry not found.
 */
FSStatus
FSGetStat(FSClient *client,
          FSCmdBlock *block,
          const char *path,
          FSStat *returnedStat,
          FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSGetStatAsync(client, block, path, returnedStat,
                                errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Get statistics about a filesystem entry (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetStatAsync(FSClient *client,
               FSCmdBlock *block,
               const char *path,
               FSStat *returnedStat,
               FSErrorFlag errorMask,
               const FSAsyncData *asyncData)
{
   return internal::getInfoByQueryAsync(client, block, path,
                                        FSQueryInfoType::Stat, returnedStat,
                                        errorMask, asyncData);
}


/**
 * Get statistics about an opened file.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetStatFile(FSClient *client,
              FSCmdBlock *block,
              FSFileHandle handle,
              FSStat *returnedStat,
              FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSGetStatFileAsync(client, block, handle, returnedStat,
                                    errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Get statistics about an opened file (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetStatFileAsync(FSClient *client,
                   FSCmdBlock *block,
                   FSFileHandle handle,
                   FSStat *returnedStat,
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

   blockBody->cmdData.statFile.stat = returnedStat;

   auto error = internal::fsaShimPrepareRequestStatFile(&blockBody->fsaShimBuffer,
                                                        clientBody->clientHandle,
                                                        handle);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::fsCmdBlockFinishCmdFn);
   return FSStatus::OK;
}


/**
 * Checks if current file position is at the end of the file.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 *
 * \retval FSStatus::OK
 * Returns FSStatus::OK when not at the end of the file.
 *
 * \retval FSStatus::End
 * Returns FSStatus::End when at the end of the file.
 */
FSStatus
FSIsEof(FSClient *client,
        FSCmdBlock *block,
        FSFileHandle handle,
        FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSIsEofAsync(client, block, handle, errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Checks if current file position is at the end of the file (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSIsEofAsync(FSClient *client,
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

   auto error = internal::fsaShimPrepareRequestIsEof(&blockBody->fsaShimBuffer,
                                                     clientBody->clientHandle,
                                                     handle);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::fsCmdBlockFinishCmdFn);
   return FSStatus::OK;
}


/**
 * Create a directory.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSMakeDir(FSClient *client,
          FSCmdBlock *block,
          const char *path,
          FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSMakeDirAsync(client, block, path,
                                errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Create a directory (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSMakeDirAsync(FSClient *client,
               FSCmdBlock *block,
               const char *path,
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

   if (!path) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidPath);
      return FSStatus::FatalError;
   }

   auto error = internal::fsaShimPrepareRequestMakeDir(&blockBody->fsaShimBuffer,
                                                       clientBody->clientHandle,
                                                       path, 0x660);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::fsCmdBlockFinishCmdFn);
   return FSStatus::OK;
}


/**
 * Open a directory for iterating it's content.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 *
 * \retval FSStatus::NotFound
 * Directory not found.
 *
 * \retval FSStatus::NotDirectory
 * Used OpenDir on a non-directory object such as a file.
 *
 * \retval FSStatus::PermissionError
 * Did not have permission to open the directory in specified mode.
 */
FSStatus
FSOpenDir(FSClient *client,
          FSCmdBlock *block,
          const char *path,
          be_val<FSDirHandle> *dirHandle,
          FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSOpenDirAsync(client, block, path, dirHandle,
                                errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Open a directory for iterating it's content (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSOpenDirAsync(FSClient *client,
               FSCmdBlock *block,
               const char *path,
               be_val<FSDirHandle> *dirHandle,
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

   if (!dirHandle) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidBuffer);
      return FSStatus::FatalError;
   }

   if (!path) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidPath);
      return FSStatus::FatalError;
   }

   blockBody->cmdData.openDir.handle = dirHandle;
   auto error = internal::fsaShimPrepareRequestOpenDir(&blockBody->fsaShimBuffer,
                                                        clientBody->clientHandle,
                                                        path);

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
 * Read the next entry in a directory.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSReadDir(FSClient *client,
          FSCmdBlock *block,
          FSDirHandle handle,
          FSDirEntry *returnedDirEntry,
          FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSReadDirAsync(client, block, handle, returnedDirEntry,
                                errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Read the next entry in a directory (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSReadDirAsync(FSClient *client,
               FSCmdBlock *block,
               FSDirHandle handle,
               FSDirEntry *returnedDirEntry,
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

   if (!returnedDirEntry) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidBuffer);
      return FSStatus::FatalError;
   }

   blockBody->cmdData.readDir.entry = returnedDirEntry;

   auto error = internal::fsaShimPrepareRequestReadDir(&blockBody->fsaShimBuffer,
                                                       clientBody->clientHandle,
                                                       handle);

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
                const FSAsyncData *asyncData)
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
                       const FSAsyncData *asyncData)
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
              const FSAsyncData *asyncData)
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
 * Rename a file or directory (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 *
 * \retval FSStatus::NotFound
 * Entry not found.
 */
FSStatus
FSRename(FSClient *client,
         FSCmdBlock *block,
         const char *oldPath,
         const char *newPath,
         FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSRenameAsync(client, block, oldPath, newPath,
                               errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block,
                                              result, errorMask);
}


/**
 * Rename a file or directory (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSRenameAsync(FSClient *client,
              FSCmdBlock *block,
              const char *oldPath,
              const char *newPath,
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

   if (!oldPath) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidPath);
      return FSStatus::FatalError;
   }

   if (!newPath) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidPath);
      return FSStatus::FatalError;
   }

   auto error = internal::fsaShimPrepareRequestRename(&blockBody->fsaShimBuffer,
                                                      clientBody->clientHandle,
                                                      oldPath,
                                                      newPath);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::fsCmdBlockFinishCmdFn);
   return FSStatus::OK;
}


/**
 * Rewind the read directory iterator back to the beginning.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSRewindDir(FSClient *client,
            FSCmdBlock *block,
            FSDirHandle handle,
            FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSRewindDirAsync(client, block, handle, errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block,
                                              result, errorMask);
}


/**
 * Rewind the read directory iterator back to the beginning (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSRewindDirAsync(FSClient *client,
                 FSCmdBlock *block,
                 FSDirHandle handle,
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

   auto error = internal::fsaShimPrepareRequestRewindDir(&blockBody->fsaShimBuffer,
                                                         clientBody->clientHandle,
                                                         handle);

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
                  const FSAsyncData *asyncData)
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


/**
 * Truncate a file to it's current position.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSTruncateFile(FSClient *client,
               FSCmdBlock *block,
               FSFileHandle handle,
               FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSTruncateFileAsync(client, block, handle, errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Truncate a file to it's current position (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSTruncateFileAsync(FSClient *client,
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

   auto error = internal::fsaShimPrepareRequestTruncateFile(&blockBody->fsaShimBuffer,
                                                            clientBody->clientHandle,
                                                            handle);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::fsCmdBlockFinishCmdFn);
   return FSStatus::OK;
}


/**
 * Write to a file.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSWriteFile(FSClient *client,
            FSCmdBlock *block,
            const uint8_t *buffer,
            uint32_t size,
            uint32_t count,
            FSFileHandle handle,
            FSWriteFlag writeFlags,
            FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSWriteFileAsync(client, block, buffer, size, count, handle,
                                  writeFlags, errorMask, &asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Write to a file (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSWriteFileAsync(FSClient *client,
                 FSCmdBlock *block,
                 const uint8_t *buffer,
                 uint32_t size,
                 uint32_t count,
                 FSFileHandle handle,
                 FSWriteFlag writeFlags,
                 FSErrorFlag errorMask,
                 const FSAsyncData *asyncData)
{
   return internal::writeFileWithPosAsync(client, block, buffer, size, count,
                                          0, handle,
                                          static_cast<FSWriteFlag>(writeFlags & ~FSWriteFlag::WriteWithPos),
                                          errorMask, asyncData);
}


/**
 * Write to a file at a specific position.
 *
 * The files position will be set before writing.
 *
 * This is equivalent to:
 *    FSSetPosFile(file, pos)
 *    FSWriteFile(file, ...)
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSWriteFileWithPos(FSClient *client,
                   FSCmdBlock *block,
                   const uint8_t *buffer,
                   uint32_t size,
                   uint32_t count,
                   FSFilePosition pos,
                   FSFileHandle handle,
                   FSWriteFlag writeFlags,
                   FSErrorFlag errorMask)
{
   FSAsyncData asyncData;
   internal::fsCmdBlockPrepareSync(client, block, &asyncData);

   auto result = FSWriteFileWithPosAsync(client, block, buffer, size, count,
                                         handle, pos, writeFlags, errorMask,
                                         &asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Write to a file at a specific position (asynchronously).
 *
 * The files position will be set before writing.
 *
 * This is equivalent to:
 *    FSSetPosFile(file, pos)
 *    FSWriteFile(file, ...)
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSWriteFileWithPosAsync(FSClient *client,
                        FSCmdBlock *block,
                        const uint8_t *buffer,
                        uint32_t size,
                        uint32_t count,
                        FSFilePosition pos,
                        FSFileHandle handle,
                        FSWriteFlag writeFlags,
                        FSErrorFlag errorMask,
                        const FSAsyncData *asyncData)
{
   return internal::writeFileWithPosAsync(client, block, buffer, size, count,
                                          0, handle,
                                          static_cast<FSWriteFlag>(writeFlags | FSWriteFlag::WriteWithPos),
                                          errorMask, asyncData);
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
                     const FSAsyncData *asyncData)
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
                                                        1,
                                                        blockBody->cmdData.readFile.readSize,
                                                        pos,
                                                        handle,
                                                        readFlags);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::fsCmdBlockFinishReadCmdFn);
   return FSStatus::OK;
}

static FSStatus
getInfoByQueryAsync(FSClient *client,
                    FSCmdBlock *block,
                    const char *path,
                    FSQueryInfoType type,
                    void *out,
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

   if (!path) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidPath);
      return FSStatus::FatalError;
   }

   if (!out) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidBuffer);
      return FSStatus::FatalError;
   }

   blockBody->cmdData.getInfoByQuery.out = out;

   auto error = internal::fsaShimPrepareRequestGetInfoByQuery(&blockBody->fsaShimBuffer,
                                                              clientBody->clientHandle,
                                                              path, type);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::fsCmdBlockFinishReadCmdFn);
   return FSStatus::OK;
}

FSStatus
writeFileWithPosAsync(FSClient *client,
                      FSCmdBlock *block,
                      const uint8_t *buffer,
                      uint32_t size,
                      uint32_t count,
                      FSFilePosition pos,
                      FSFileHandle handle,
                      FSWriteFlag writeFlags,
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

   // Ensure size * count is not > 32 bit.
   auto bytes = uint64_t { size } * uint64_t { count };

   if (bytes > 0xFFFFFFFFull) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidParam);
      return FSStatus::FatalError;
   }

   blockBody->cmdData.writeFile.chunkSize = size;
   blockBody->cmdData.writeFile.bytesRemaining = size * count;
   blockBody->cmdData.writeFile.bytesWritten = 0;

   // We only read up to FSMaxBytesPerRequest per request.
   if (size > FSMaxBytesPerRequest) {
      blockBody->cmdData.writeFile.writeSize = FSMaxBytesPerRequest;
   } else {
      blockBody->cmdData.writeFile.writeSize = size;
   }

   auto error = internal::fsaShimPrepareRequestWriteFile(&blockBody->fsaShimBuffer,
                                                         clientBody->clientHandle,
                                                         buffer,
                                                         1,
                                                         blockBody->cmdData.writeFile.writeSize,
                                                         pos,
                                                         handle,
                                                         writeFlags);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::fsCmdBlockFinishWriteCmdFn);
   return FSStatus::OK;
}

} // namespace internal

void
Module::registerFsCmdFunctions()
{
   RegisterKernelFunction(FSChangeDir);
   RegisterKernelFunction(FSChangeDirAsync);
   RegisterKernelFunction(FSCloseDir);
   RegisterKernelFunction(FSCloseDirAsync);
   RegisterKernelFunction(FSCloseFile);
   RegisterKernelFunction(FSCloseFileAsync);
   RegisterKernelFunction(FSFlushFile);
   RegisterKernelFunction(FSFlushFileAsync);
   RegisterKernelFunction(FSFlushQuota);
   RegisterKernelFunction(FSFlushQuotaAsync);
   RegisterKernelFunction(FSGetCwd);
   RegisterKernelFunction(FSGetCwdAsync);
   RegisterKernelFunction(FSGetFreeSpaceSize);
   RegisterKernelFunction(FSGetFreeSpaceSizeAsync);
   RegisterKernelFunction(FSGetPosFile);
   RegisterKernelFunction(FSGetPosFileAsync);
   RegisterKernelFunction(FSGetStat);
   RegisterKernelFunction(FSGetStatAsync);
   RegisterKernelFunction(FSGetStatFile);
   RegisterKernelFunction(FSGetStatFileAsync);
   RegisterKernelFunction(FSIsEof);
   RegisterKernelFunction(FSIsEofAsync);
   RegisterKernelFunction(FSMakeDir);
   RegisterKernelFunction(FSMakeDirAsync);
   RegisterKernelFunction(FSOpenDir);
   RegisterKernelFunction(FSOpenDirAsync);
   RegisterKernelFunction(FSOpenFile);
   RegisterKernelFunction(FSOpenFileAsync);
   RegisterKernelFunction(FSOpenFileEx);
   RegisterKernelFunction(FSOpenFileExAsync);
   RegisterKernelFunction(FSReadDir);
   RegisterKernelFunction(FSReadDirAsync);
   RegisterKernelFunction(FSReadFile);
   RegisterKernelFunction(FSReadFileAsync);
   RegisterKernelFunction(FSReadFileWithPos);
   RegisterKernelFunction(FSReadFileWithPosAsync);
   RegisterKernelFunction(FSRemove);
   RegisterKernelFunction(FSRemoveAsync);
   RegisterKernelFunction(FSRename);
   RegisterKernelFunction(FSRenameAsync);
   RegisterKernelFunction(FSRewindDir);
   RegisterKernelFunction(FSRewindDirAsync);
   RegisterKernelFunction(FSSetPosFile);
   RegisterKernelFunction(FSSetPosFileAsync);
   RegisterKernelFunction(FSTruncateFile);
   RegisterKernelFunction(FSTruncateFileAsync);
   RegisterKernelFunction(FSWriteFile);
   RegisterKernelFunction(FSWriteFileAsync);
   RegisterKernelFunction(FSWriteFileWithPos);
   RegisterKernelFunction(FSWriteFileWithPosAsync);
}

} // namespace coreinit
