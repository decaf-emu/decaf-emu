#include "coreinit.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_cmd.h"
#include "coreinit_fs_cmdblock.h"
#include "coreinit_fsa_shim.h"
#include "cafe/cafe_stackobject.h"

namespace cafe::coreinit
{

namespace internal
{

static FSStatus
readFileWithPosAsync(virt_ptr<FSClient> client,
                     virt_ptr<FSCmdBlock> block,
                     virt_ptr<uint8_t> buffer,
                     uint32_t size,
                     uint32_t count,
                     FSFilePosition pos,
                     FSFileHandle handle,
                     FSReadFlag readFlags,
                     FSErrorFlag errorMask,
                     virt_ptr<const FSAsyncData> asyncData);

static FSStatus
writeFileWithPosAsync(virt_ptr<FSClient> client,
                      virt_ptr<FSCmdBlock> block,
                      virt_ptr<const uint8_t> buffer,
                      uint32_t size,
                      uint32_t count,
                      FSFilePosition pos,
                      FSFileHandle handle,
                      FSWriteFlag writeFlags,
                      FSErrorFlag errorMask,
                      virt_ptr<const FSAsyncData> asyncData);

static FSStatus
getInfoByQueryAsync(virt_ptr<FSClient> client,
                    virt_ptr<FSCmdBlock> block,
                    virt_ptr<const char> path,
                    FSAQueryInfoType type,
                    virt_ptr<void> out,
                    FSErrorFlag errorMask,
                    virt_ptr<const FSAsyncData> asyncData);

} // namespace internal


/**
 * Change the client's working directory.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSChangeDir(virt_ptr<FSClient> client,
            virt_ptr<FSCmdBlock> block,
            virt_ptr<const char> path,
            FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSChangeDirAsync(client, block, path, errorMask, asyncData);

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
FSChangeDirAsync(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 virt_ptr<const char> path,
                 FSErrorFlag errorMask,
                 virt_ptr<const FSAsyncData> asyncData)
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

   auto error = internal::fsaShimPrepareRequestChangeDir(virt_addrof(blockBody->fsaShimBuffer),
                                                         clientBody->clientHandle,
                                                         path);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
   return FSStatus::OK;
}


/**
 * Close a directory.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSCloseDir(virt_ptr<FSClient> client,
           virt_ptr<FSCmdBlock> block,
           FSDirHandle handle,
           FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSCloseDirAsync(client, block, handle, errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Close a directory (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSCloseDirAsync(virt_ptr<FSClient> client,
                virt_ptr<FSCmdBlock> block,
                FSDirHandle handle,
                FSErrorFlag errorMask,
                virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   auto error = internal::fsaShimPrepareRequestCloseDir(virt_addrof(blockBody->fsaShimBuffer),
                                                        clientBody->clientHandle,
                                                        handle);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
   return FSStatus::OK;
}


/**
 * Close a file.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSCloseFile(virt_ptr<FSClient> client,
            virt_ptr<FSCmdBlock> block,
            FSFileHandle handle,
            FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSCloseFileAsync(client, block, handle, errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Close a file (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSCloseFileAsync(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 FSFileHandle handle,
                 FSErrorFlag errorMask,
                 virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   auto error = internal::fsaShimPrepareRequestCloseFile(virt_addrof(blockBody->fsaShimBuffer),
                                                         clientBody->clientHandle,
                                                         handle);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
   return FSStatus::OK;
}


/**
 * Flush the contents of a file to disk.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSFlushFile(virt_ptr<FSClient> client,
            virt_ptr<FSCmdBlock> block,
            FSFileHandle handle,
            FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSFlushFileAsync(client, block, handle, errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Flush the contents of a file to disk (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSFlushFileAsync(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 FSFileHandle handle,
                 FSErrorFlag errorMask,
                 virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   auto error = internal::fsaShimPrepareRequestFlushFile(virt_addrof(blockBody->fsaShimBuffer),
                                                         clientBody->clientHandle,
                                                         handle);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
   return FSStatus::OK;
}


/**
 * I don't know what this does :).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSFlushQuota(virt_ptr<FSClient> client,
             virt_ptr<FSCmdBlock> block,
             virt_ptr<const char> path,
             FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSFlushQuotaAsync(client, block, path, errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * I don't know what this does :) (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSFlushQuotaAsync(virt_ptr<FSClient> client,
                  virt_ptr<FSCmdBlock> block,
                  virt_ptr<const char> path,
                  FSErrorFlag errorMask,
                  virt_ptr<const FSAsyncData> asyncData)
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

   auto error = internal::fsaShimPrepareRequestFlushQuota(virt_addrof(blockBody->fsaShimBuffer),
                                                          clientBody->clientHandle,
                                                          path);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
   return FSStatus::OK;
}


/**
 * Get the current working directory.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetCwd(virt_ptr<FSClient> client,
         virt_ptr<FSCmdBlock> block,
         virt_ptr<char> returnedPath,
         uint32_t bytes,
         FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSGetCwdAsync(client, block, returnedPath, bytes,
                               errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Get the current working directory (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetCwdAsync(virt_ptr<FSClient> client,
              virt_ptr<FSCmdBlock> block,
              virt_ptr<char> returnedPath,
              uint32_t bytes,
              FSErrorFlag errorMask,
              virt_ptr<const FSAsyncData> asyncData)
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

   auto error = internal::fsaShimPrepareRequestGetCwd(virt_addrof(blockBody->fsaShimBuffer),
                                                      clientBody->clientHandle);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
   return FSStatus::OK;
}


/**
 * Get directory size.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 *
 * \retval FSStatus::NotFound
 * Directory not found.
 */
FSStatus
FSGetDirSize(virt_ptr<FSClient> client,
             virt_ptr<FSCmdBlock> block,
             virt_ptr<const char> path,
             virt_ptr<uint64_t> outDirSize,
             FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSGetDirSizeAsync(client, block, path, outDirSize,
                                   errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Get directory size.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetDirSizeAsync(virt_ptr<FSClient> client,
                  virt_ptr<FSCmdBlock> block,
                  virt_ptr<const char> path,
                  virt_ptr<uint64_t> outDirSize,
                  FSErrorFlag errorMask,
                  virt_ptr<const FSAsyncData> asyncData)
{
   return internal::getInfoByQueryAsync(client, block, path,
                                        FSAQueryInfoType::DirSize,
                                        outDirSize,
                                        errorMask, asyncData);
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
FSGetFreeSpaceSize(virt_ptr<FSClient> client,
                   virt_ptr<FSCmdBlock> block,
                   virt_ptr<const char> path,
                   virt_ptr<uint64_t> outFreeSize,
                   FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSGetFreeSpaceSizeAsync(client, block, path, outFreeSize,
                                         errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Get free space for entry at path.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetFreeSpaceSizeAsync(virt_ptr<FSClient> client,
                        virt_ptr<FSCmdBlock> block,
                        virt_ptr<const char> path,
                        virt_ptr<uint64_t> outFreeSize,
                        FSErrorFlag errorMask,
                        virt_ptr<const FSAsyncData> asyncData)
{
   return internal::getInfoByQueryAsync(client, block, path,
                                        FSAQueryInfoType::FreeSpaceSize,
                                        outFreeSize,
                                        errorMask, asyncData);
}


/**
 * Get the first mount source which matches the specified type.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetMountSource(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 FSMountSourceType type,
                 virt_ptr<FSMountSource> source,
                 FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSGetMountSourceAsync(client, block, type, source, errorMask,
                                       asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Get the first mount source which matches the specified type (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetMountSourceAsync(virt_ptr<FSClient> client,
                      virt_ptr<FSCmdBlock> block,
                      FSMountSourceType type,
                      virt_ptr<FSMountSource> source,
                      FSErrorFlag errorMask,
                      virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);

   if (!clientBody) {
      return FSStatus::FatalError;
   }

   if (type != FSMountSourceType::SdCard && type != FSMountSourceType::HostFileIO) {
      return FSStatus::FatalError;
   }

   clientBody->lastMountSourceDevice[0] = char { 0 };
   clientBody->findMountSourceType = type;

   return FSGetMountSourceNextAsync(client, block, source, errorMask, asyncData);
}


/**
 * Get the next mount source.
 *
 * This can be called repeatedly after FSGetMountSource until failure.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 *
 * \retval FSStatus::End
 * Returned when we have iterated over all the mount sources for this type.
 */
FSStatus
FSGetMountSourceNext(virt_ptr<FSClient> client,
                     virt_ptr<FSCmdBlock> block,
                     virt_ptr<FSMountSource> source,
                     FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSGetMountSourceNextAsync(client, block, source, errorMask,
                                           asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Get the next mount source (asynchronously).
 *
 * This can be called repeatedly after FSGetMountSource until failure.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetMountSourceNextAsync(virt_ptr<FSClient> client,
                          virt_ptr<FSCmdBlock> block,
                          virt_ptr<FSMountSource> source,
                          FSErrorFlag errorMask,
                          virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   if (!source) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidBuffer);
      return FSStatus::FatalError;
   }

   blockBody->cmdData.getMountSourceNext.source = source;
   blockBody->cmdData.getMountSourceNext.dirHandle = -1;
   auto error = internal::fsaShimPrepareRequestOpenDir(virt_addrof(blockBody->fsaShimBuffer),
                                                       clientBody->clientHandle,
                                                       make_stack_string("/dev"));

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody,
                                   blockBody,
                                   internal::FinishGetMountSourceNextOpenCmd);
   return FSStatus::OK;
}


/**
 * Get the current read / write position of a file.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetPosFile(virt_ptr<FSClient> client,
             virt_ptr<FSCmdBlock> block,
             FSFileHandle handle,
             virt_ptr<FSFilePosition> outPos,
             FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSGetPosFileAsync(client, block, handle, outPos,
                                   errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Get the current read / write position of a file (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetPosFileAsync(virt_ptr<FSClient> client,
                  virt_ptr<FSCmdBlock> block,
                  FSFileHandle handle,
                  virt_ptr<FSFilePosition> outPos,
                  FSErrorFlag errorMask,
                  virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   if (!outPos) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidBuffer);
      return FSStatus::FatalError;
   }

   blockBody->cmdData.getPosFile.pos = outPos;
   auto error = internal::fsaShimPrepareRequestGetPosFile(virt_addrof(blockBody->fsaShimBuffer),
                                                          clientBody->clientHandle,
                                                          handle);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
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
FSGetStat(virt_ptr<FSClient> client,
          virt_ptr<FSCmdBlock> block,
          virt_ptr<const char> path,
          virt_ptr<FSStat> outStat,
          FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSGetStatAsync(client, block, path, outStat,
                                errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Get statistics about a filesystem entry (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetStatAsync(virt_ptr<FSClient> client,
               virt_ptr<FSCmdBlock> block,
               virt_ptr<const char> path,
               virt_ptr<FSStat> outStat,
               FSErrorFlag errorMask,
               virt_ptr<const FSAsyncData> asyncData)
{
   return internal::getInfoByQueryAsync(client, block, path,
                                        FSAQueryInfoType::Stat, outStat,
                                        errorMask, asyncData);
}


/**
 * Get statistics about an opened file.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetStatFile(virt_ptr<FSClient> client,
              virt_ptr<FSCmdBlock> block,
              FSFileHandle handle,
              virt_ptr<FSStat> outStat,
              FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSGetStatFileAsync(client, block, handle, outStat,
                                    errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Get statistics about an opened file (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSGetStatFileAsync(virt_ptr<FSClient> client,
                   virt_ptr<FSCmdBlock> block,
                   FSFileHandle handle,
                   virt_ptr<FSStat> outStat,
                   FSErrorFlag errorMask,
                   virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   blockBody->cmdData.statFile.stat = outStat;

   auto error = internal::fsaShimPrepareRequestStatFile(virt_addrof(blockBody->fsaShimBuffer),
                                                        clientBody->clientHandle,
                                                        handle);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
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
FSIsEof(virt_ptr<FSClient> client,
        virt_ptr<FSCmdBlock> block,
        FSFileHandle handle,
        FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSIsEofAsync(client, block, handle, errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Checks if current file position is at the end of the file (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSIsEofAsync(virt_ptr<FSClient> client,
             virt_ptr<FSCmdBlock> block,
             FSFileHandle handle,
             FSErrorFlag errorMask,
             virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   auto error = internal::fsaShimPrepareRequestIsEof(virt_addrof(blockBody->fsaShimBuffer),
                                                     clientBody->clientHandle,
                                                     handle);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
   return FSStatus::OK;
}


/**
 * Create a directory.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSMakeDir(virt_ptr<FSClient> client,
          virt_ptr<FSCmdBlock> block,
          virt_ptr<const char> path,
          FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSMakeDirAsync(client, block, path,
                                errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Create a directory (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSMakeDirAsync(virt_ptr<FSClient> client,
               virt_ptr<FSCmdBlock> block,
               virt_ptr<const char> path,
               FSErrorFlag errorMask,
               virt_ptr<const FSAsyncData> asyncData)
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

   auto error = internal::fsaShimPrepareRequestMakeDir(virt_addrof(blockBody->fsaShimBuffer),
                                                       clientBody->clientHandle,
                                                       path, 0x660);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
   return FSStatus::OK;
}


/**
 * Mount a mount source.
 *
 * The mounted path is returned in target.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSMount(virt_ptr<FSClient> client,
        virt_ptr<FSCmdBlock> block,
        virt_ptr<FSMountSource> source,
        virt_ptr<char> target,
        uint32_t bytes,
        FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSMountAsync(client, block, source, target, bytes,
                              errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Mount a mount source (asynchronously).
 *
 * The mounted path is returned in target.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSMountAsync(virt_ptr<FSClient> client,
             virt_ptr<FSCmdBlock> block,
             virt_ptr<FSMountSource> source,
             virt_ptr<char> target,
             uint32_t bytes,
             FSErrorFlag errorMask,
             virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  static_cast<FSErrorFlag>(errorMask | FSErrorFlag::Exists),
                                                  asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   if (!source || !target || bytes < FSMaxMountPathLength) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidBuffer);
      return FSStatus::FatalError;
   }

   if (source->sourceType != FSMountSourceType::SdCard &&
       source->sourceType != FSMountSourceType::HostFileIO) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidParam);
      return FSStatus::FatalError;
   }

   // Set target path as /vol/<source path>
   std::memcpy(target.getRawPointer(), "/vol/", 5);
   std::strncpy(target.getRawPointer() + 5,
                virt_addrof(source->path).getRawPointer(),
                bytes - 4);

   auto error = internal::fsaShimPrepareRequestMount(virt_addrof(blockBody->fsaShimBuffer),
                                                     clientBody->clientHandle,
                                                     virt_addrof(source->path),
                                                     target,
                                                     0,
                                                     nullptr, 0);

   // Correct the device path.
   auto devicePath = virt_addrof(blockBody->fsaShimBuffer.request.mount.path);
   auto sourcePath = virt_addrof(source->path);

   if (strncmp(sourcePath.getRawPointer(), "external", 8) == 0) {
      // external01 to /dev/sdcard01
      std::memcpy(devicePath.getRawPointer(), "/dev/sdcard", 11);
      std::strncpy(devicePath.getRawPointer() + 11,
                   sourcePath.getRawPointer() + 8,
                   2);
   } else {
      // <source path> to /dev/<source path>
      std::memcpy(devicePath.getRawPointer(), "/dev/", 5);
      std::strncpy(devicePath.getRawPointer() + 5,
                   sourcePath.getRawPointer(),
                   FSMaxPathLength - 4);
   }

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody,
                                   blockBody,
                                   internal::FinishMountCmd);
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
FSOpenDir(virt_ptr<FSClient> client,
          virt_ptr<FSCmdBlock> block,
          virt_ptr<const char> path,
          virt_ptr<FSDirHandle> outHandle,
          FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSOpenDirAsync(client, block, path, outHandle,
                                errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Open a directory for iterating it's content (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSOpenDirAsync(virt_ptr<FSClient> client,
               virt_ptr<FSCmdBlock> block,
               virt_ptr<const char> path,
               virt_ptr<FSDirHandle> outHandle,
               FSErrorFlag errorMask,
               virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   if (!outHandle) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidBuffer);
      return FSStatus::FatalError;
   }

   if (!path) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidPath);
      return FSStatus::FatalError;
   }

   blockBody->cmdData.openDir.handle = outHandle;
   auto error = internal::fsaShimPrepareRequestOpenDir(virt_addrof(blockBody->fsaShimBuffer),
                                                        clientBody->clientHandle,
                                                        path);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
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
FSOpenFile(virt_ptr<FSClient> client,
           virt_ptr<FSCmdBlock> block,
           virt_ptr<const char> path,
           virt_ptr<const char> mode,
           virt_ptr<FSFileHandle> outHandle,
           FSErrorFlag errorMask)
{
   return FSOpenFileEx(client, block, path, mode,
                       0x660, 0, 0,
                       outHandle, errorMask);
}


/**
 * Open a file (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSOpenFileAsync(virt_ptr<FSClient> client,
                virt_ptr<FSCmdBlock> block,
                virt_ptr<const char> path,
                virt_ptr<const char> mode,
                virt_ptr<FSFileHandle> outHandle,
                FSErrorFlag errorMask,
                virt_ptr<const FSAsyncData> asyncData)
{
   return FSOpenFileExAsync(client, block, path, mode,
                            0x660, 0, 0,
                            outHandle, errorMask, asyncData);
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
FSOpenFileEx(virt_ptr<FSClient> client,
             virt_ptr<FSCmdBlock> block,
             virt_ptr<const char> path,
             virt_ptr<const char> mode,
             uint32_t unk1,
             uint32_t unk2,
             uint32_t unk3,
             virt_ptr<FSFileHandle> outHandle,
             FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSOpenFileExAsync(client, block, path, mode,
                                   unk1, unk2, unk3,
                                   outHandle, errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Open a file (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSOpenFileExAsync(virt_ptr<FSClient> client,
                  virt_ptr<FSCmdBlock> block,
                  virt_ptr<const char> path,
                  virt_ptr<const char> mode,
                  uint32_t unk1,
                  uint32_t unk2,
                  uint32_t unk3,
                  virt_ptr<FSFileHandle> outHandle,
                  FSErrorFlag errorMask,
                  virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   if (!outHandle) {
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

   blockBody->cmdData.openFile.handle = outHandle;
   auto error = internal::fsaShimPrepareRequestOpenFile(virt_addrof(blockBody->fsaShimBuffer),
                                                        clientBody->clientHandle,
                                                        path, mode,
                                                        unk1, unk2, unk3);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
   return FSStatus::OK;
}


/**
 * Read the next entry in a directory.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSReadDir(virt_ptr<FSClient> client,
          virt_ptr<FSCmdBlock> block,
          FSDirHandle handle,
          virt_ptr<FSDirEntry> outDirEntry,
          FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSReadDirAsync(client, block, handle, outDirEntry,
                                errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Read the next entry in a directory (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSReadDirAsync(virt_ptr<FSClient> client,
               virt_ptr<FSCmdBlock> block,
               FSDirHandle handle,
               virt_ptr<FSDirEntry> outDirEntry,
               FSErrorFlag errorMask,
               virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   if (!outDirEntry) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidBuffer);
      return FSStatus::FatalError;
   }

   blockBody->cmdData.readDir.entry = outDirEntry;

   auto error = internal::fsaShimPrepareRequestReadDir(virt_addrof(blockBody->fsaShimBuffer),
                                                       clientBody->clientHandle,
                                                       handle);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
   return FSStatus::OK;
}


/**
 * Read a file.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSReadFile(virt_ptr<FSClient> client,
           virt_ptr<FSCmdBlock> block,
           virt_ptr<uint8_t> buffer,
           uint32_t size,
           uint32_t count,
           FSFileHandle handle,
           FSReadFlag readFlags,
           FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSReadFileAsync(client, block, buffer, size, count, handle,
                                 readFlags, errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Read a file (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSReadFileAsync(virt_ptr<FSClient> client,
                virt_ptr<FSCmdBlock> block,
                virt_ptr<uint8_t> buffer,
                uint32_t size,
                uint32_t count,
                FSFileHandle handle,
                FSReadFlag readFlags,
                FSErrorFlag errorMask,
                virt_ptr<const FSAsyncData> asyncData)
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
FSReadFileWithPos(virt_ptr<FSClient> client,
                  virt_ptr<FSCmdBlock> block,
                  virt_ptr<uint8_t> buffer,
                  uint32_t size,
                  uint32_t count,
                  FSFilePosition pos,
                  FSFileHandle handle,
                  FSReadFlag readFlags,
                  FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSReadFileWithPosAsync(client, block, buffer, size, count,
                                        pos, handle, readFlags, errorMask,
                                        asyncData);

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
FSReadFileWithPosAsync(virt_ptr<FSClient> client,
                       virt_ptr<FSCmdBlock> block,
                       virt_ptr<uint8_t> buffer,
                       uint32_t size,
                       uint32_t count,
                       FSFilePosition pos,
                       FSFileHandle handle,
                       FSReadFlag readFlags,
                       FSErrorFlag errorMask,
                       virt_ptr<const FSAsyncData> asyncData)
{
   return internal::readFileWithPosAsync(client, block, buffer, size, count,
                                         pos, handle,
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
FSRemove(virt_ptr<FSClient> client,
         virt_ptr<FSCmdBlock> block,
         virt_ptr<const char> path,
         FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSRemoveAsync(client, block, path, errorMask, asyncData);

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
FSRemoveAsync(virt_ptr<FSClient> client,
              virt_ptr<FSCmdBlock> block,
              virt_ptr<const char> path,
              FSErrorFlag errorMask,
              virt_ptr<const FSAsyncData> asyncData)
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

   auto error = internal::fsaShimPrepareRequestRemove(virt_addrof(blockBody->fsaShimBuffer),
                                                      clientBody->clientHandle,
                                                      path);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
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
FSRename(virt_ptr<FSClient> client,
         virt_ptr<FSCmdBlock> block,
         virt_ptr<const char> oldPath,
         virt_ptr<const char> newPath,
         FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSRenameAsync(client, block, oldPath, newPath,
                               errorMask, asyncData);

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
FSRenameAsync(virt_ptr<FSClient> client,
              virt_ptr<FSCmdBlock> block,
              virt_ptr<const char> oldPath,
              virt_ptr<const char> newPath,
              FSErrorFlag errorMask,
              virt_ptr<const FSAsyncData> asyncData)
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

   auto error = internal::fsaShimPrepareRequestRename(virt_addrof(blockBody->fsaShimBuffer),
                                                      clientBody->clientHandle,
                                                      oldPath,
                                                      newPath);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
   return FSStatus::OK;
}


/**
 * Rewind the read directory iterator back to the beginning.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSRewindDir(virt_ptr<FSClient> client,
            virt_ptr<FSCmdBlock> block,
            FSDirHandle handle,
            FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSRewindDirAsync(client, block, handle, errorMask, asyncData);

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
FSRewindDirAsync(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 FSDirHandle handle,
                 FSErrorFlag errorMask,
                 virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   auto error = internal::fsaShimPrepareRequestRewindDir(virt_addrof(blockBody->fsaShimBuffer),
                                                         clientBody->clientHandle,
                                                         handle);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
   return FSStatus::OK;
}


/**
 * Set the current read / write position for a file.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSSetPosFile(virt_ptr<FSClient> client,
             virt_ptr<FSCmdBlock> block,
             FSFileHandle handle,
             FSFilePosition pos,
             FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSSetPosFileAsync(client, block, handle, pos,
                                   errorMask, asyncData);

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
FSSetPosFileAsync(virt_ptr<FSClient> client,
                  virt_ptr<FSCmdBlock> block,
                  FSFileHandle handle,
                  FSFilePosition pos,
                  FSErrorFlag errorMask,
                  virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   auto error = internal::fsaShimPrepareRequestSetPosFile(virt_addrof(blockBody->fsaShimBuffer),
                                                          clientBody->clientHandle,
                                                          handle,
                                                          pos);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
   return FSStatus::OK;
}


/**
 * Truncate a file to it's current position.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSTruncateFile(virt_ptr<FSClient> client,
               virt_ptr<FSCmdBlock> block,
               FSFileHandle handle,
               FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSTruncateFileAsync(client, block, handle, errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Truncate a file to it's current position (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSTruncateFileAsync(virt_ptr<FSClient> client,
                    virt_ptr<FSCmdBlock> block,
                    FSFileHandle handle,
                    FSErrorFlag errorMask,
                    virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   auto error = internal::fsaShimPrepareRequestTruncateFile(virt_addrof(blockBody->fsaShimBuffer),
                                                            clientBody->clientHandle,
                                                            handle);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
   return FSStatus::OK;
}


/**
 * Unmount a target which was previously mounted with FSMount.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSUnmount(virt_ptr<FSClient> client,
          virt_ptr<FSCmdBlock> block,
          virt_ptr<const char> target,
          FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSUnmountAsync(client, block, target, errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Unmount a target which was previously mounted with FSMount (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSUnmountAsync(virt_ptr<FSClient> client,
               virt_ptr<FSCmdBlock> block,
               virt_ptr<const char> target,
               FSErrorFlag errorMask,
               virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto result = internal::fsCmdBlockPrepareAsync(clientBody, blockBody,
                                                  errorMask, asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   if (!target) {
      internal::fsClientHandleFatalError(clientBody, FSAStatus::InvalidPath);
      return FSStatus::FatalError;
   }

   auto error = internal::fsaShimPrepareRequestUnmount(virt_addrof(blockBody->fsaShimBuffer),
                                                       clientBody->clientHandle,
                                                       target,
                                                       0x80000000);

   if (error) {
      return internal::fsClientHandleShimPrepareError(clientBody, error);
   }

   internal::fsClientSubmitCommand(clientBody, blockBody, internal::FinishCmd);
   return FSStatus::OK;
}


/**
 * Write to a file.
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSWriteFile(virt_ptr<FSClient> client,
            virt_ptr<FSCmdBlock> block,
            virt_ptr<const uint8_t> buffer,
            uint32_t size,
            uint32_t count,
            FSFileHandle handle,
            FSWriteFlag writeFlags,
            FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSWriteFileAsync(client, block, buffer, size, count, handle,
                                  writeFlags, errorMask, asyncData);

   return internal::fsClientHandleAsyncResult(client, block, result, errorMask);
}


/**
 * Write to a file (asynchronously).
 *
 * \return
 * Returns negative FSStatus error code on failure, FSStatus::OK on success.
 */
FSStatus
FSWriteFileAsync(virt_ptr<FSClient> client,
                 virt_ptr<FSCmdBlock> block,
                 virt_ptr<const uint8_t> buffer,
                 uint32_t size,
                 uint32_t count,
                 FSFileHandle handle,
                 FSWriteFlag writeFlags,
                 FSErrorFlag errorMask,
                 virt_ptr<const FSAsyncData> asyncData)
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
FSWriteFileWithPos(virt_ptr<FSClient> client,
                   virt_ptr<FSCmdBlock> block,
                   virt_ptr<const uint8_t> buffer,
                   uint32_t size,
                   uint32_t count,
                   FSFilePosition pos,
                   FSFileHandle handle,
                   FSWriteFlag writeFlags,
                   FSErrorFlag errorMask)
{
   StackObject<FSAsyncData> asyncData;
   internal::fsCmdBlockPrepareSync(client, block, asyncData);

   auto result = FSWriteFileWithPosAsync(client, block, buffer, size, count,
                                         pos, handle, writeFlags, errorMask,
                                         asyncData);

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
FSWriteFileWithPosAsync(virt_ptr<FSClient> client,
                        virt_ptr<FSCmdBlock> block,
                        virt_ptr<const uint8_t> buffer,
                        uint32_t size,
                        uint32_t count,
                        FSFilePosition pos,
                        FSFileHandle handle,
                        FSWriteFlag writeFlags,
                        FSErrorFlag errorMask,
                        virt_ptr<const FSAsyncData> asyncData)
{
   return internal::writeFileWithPosAsync(client, block, buffer, size, count,
                                          pos, handle,
                                          static_cast<FSWriteFlag>(writeFlags | FSWriteFlag::WriteWithPos),
                                          errorMask, asyncData);
}


namespace internal
{

FSStatus
readFileWithPosAsync(virt_ptr<FSClient> client,
                     virt_ptr<FSCmdBlock> block,
                     virt_ptr<uint8_t> buffer,
                     uint32_t size,
                     uint32_t count,
                     FSFilePosition pos,
                     FSFileHandle handle,
                     FSReadFlag readFlags,
                     FSErrorFlag errorMask,
                     virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = fsClientGetBody(client);
   auto blockBody = fsCmdBlockGetBody(block);
   auto result = fsCmdBlockPrepareAsync(clientBody,
                                        blockBody,
                                        errorMask,
                                        asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   // Ensure size * count is not > 32 bit.
   auto bytes = uint64_t { size } * uint64_t { count };

   if (bytes > 0xFFFFFFFFull) {
      fsClientHandleFatalError(clientBody, FSAStatus::InvalidParam);
      return FSStatus::FatalError;
   }

   auto bytesRemaining = size * count;
   blockBody->cmdData.readFile.chunkSize = size;
   blockBody->cmdData.readFile.bytesRemaining = bytesRemaining;
   blockBody->cmdData.readFile.bytesRead = 0u;

   // We only read up to FSMaxBytesPerRequest per request.
   if (bytesRemaining > FSMaxBytesPerRequest) {
      blockBody->cmdData.readFile.readSize = FSMaxBytesPerRequest;
   } else {
      blockBody->cmdData.readFile.readSize = bytesRemaining;
   }

   auto error = fsaShimPrepareRequestReadFile(virt_addrof(blockBody->fsaShimBuffer),
                                              clientBody->clientHandle,
                                              buffer,
                                              1,
                                              blockBody->cmdData.readFile.readSize,
                                              pos,
                                              handle,
                                              readFlags);

   if (error) {
      return fsClientHandleShimPrepareError(clientBody, error);
   }

   fsClientSubmitCommand(clientBody, blockBody, FinishReadCmd);
   return FSStatus::OK;
}

static FSStatus
getInfoByQueryAsync(virt_ptr<FSClient> client,
                    virt_ptr<FSCmdBlock> block,
                    virt_ptr<const char> path,
                    FSAQueryInfoType type,
                    virt_ptr<void> out,
                    FSErrorFlag errorMask,
                    virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = fsClientGetBody(client);
   auto blockBody = fsCmdBlockGetBody(block);
   auto result = fsCmdBlockPrepareAsync(clientBody,
                                        blockBody,
                                        errorMask,
                                        asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   if (!path) {
      fsClientHandleFatalError(clientBody, FSAStatus::InvalidPath);
      return FSStatus::FatalError;
   }

   if (!out) {
      fsClientHandleFatalError(clientBody, FSAStatus::InvalidBuffer);
      return FSStatus::FatalError;
   }

   blockBody->cmdData.getInfoByQuery.out = out;

   auto error = fsaShimPrepareRequestGetInfoByQuery(virt_addrof(blockBody->fsaShimBuffer),
                                                    clientBody->clientHandle,
                                                    path, type);

   if (error) {
      return fsClientHandleShimPrepareError(clientBody, error);
   }

   fsClientSubmitCommand(clientBody, blockBody, FinishCmd);
   return FSStatus::OK;
}

FSStatus
writeFileWithPosAsync(virt_ptr<FSClient> client,
                      virt_ptr<FSCmdBlock> block,
                      virt_ptr<const uint8_t> buffer,
                      uint32_t size,
                      uint32_t count,
                      FSFilePosition pos,
                      FSFileHandle handle,
                      FSWriteFlag writeFlags,
                      FSErrorFlag errorMask,
                      virt_ptr<const FSAsyncData> asyncData)
{
   auto clientBody = fsClientGetBody(client);
   auto blockBody = fsCmdBlockGetBody(block);
   auto result = fsCmdBlockPrepareAsync(clientBody,
                                        blockBody,
                                        errorMask,
                                        asyncData);

   if (result != FSStatus::OK) {
      return result;
   }

   // Ensure size * count is not > 32 bit.
   auto bytes = uint64_t { size } * uint64_t { count };

   if (bytes > 0xFFFFFFFFull) {
      fsClientHandleFatalError(clientBody, FSAStatus::InvalidParam);
      return FSStatus::FatalError;
   }

   auto bytesRemaining = size * count;
   blockBody->cmdData.writeFile.chunkSize = size;
   blockBody->cmdData.writeFile.bytesRemaining = bytesRemaining;
   blockBody->cmdData.writeFile.bytesWritten = 0u;

   // We only read up to FSMaxBytesPerRequest per request.
   if (bytesRemaining > FSMaxBytesPerRequest) {
      blockBody->cmdData.writeFile.writeSize = FSMaxBytesPerRequest;
   } else {
      blockBody->cmdData.writeFile.writeSize = bytesRemaining;
   }

   auto error = fsaShimPrepareRequestWriteFile(virt_addrof(blockBody->fsaShimBuffer),
                                               clientBody->clientHandle,
                                               buffer,
                                               1,
                                               blockBody->cmdData.writeFile.writeSize,
                                               pos,
                                               handle,
                                               writeFlags);

   if (error) {
      return fsClientHandleShimPrepareError(clientBody, error);
   }

   fsClientSubmitCommand(clientBody, blockBody, FinishWriteCmd);
   return FSStatus::OK;
}

} // namespace internal

void
Library::registerFsCmdSymbols()
{
   RegisterFunctionExport(FSChangeDir);
   RegisterFunctionExport(FSChangeDirAsync);
   RegisterFunctionExport(FSCloseDir);
   RegisterFunctionExport(FSCloseDirAsync);
   RegisterFunctionExport(FSCloseFile);
   RegisterFunctionExport(FSCloseFileAsync);
   RegisterFunctionExport(FSFlushFile);
   RegisterFunctionExport(FSFlushFileAsync);
   RegisterFunctionExport(FSFlushQuota);
   RegisterFunctionExport(FSFlushQuotaAsync);
   RegisterFunctionExport(FSGetCwd);
   RegisterFunctionExport(FSGetCwdAsync);
   RegisterFunctionExport(FSGetDirSize);
   RegisterFunctionExport(FSGetDirSizeAsync);
   RegisterFunctionExport(FSGetFreeSpaceSize);
   RegisterFunctionExport(FSGetFreeSpaceSizeAsync);
   RegisterFunctionExport(FSGetPosFile);
   RegisterFunctionExport(FSGetPosFileAsync);
   RegisterFunctionExport(FSGetStat);
   RegisterFunctionExport(FSGetStatAsync);
   RegisterFunctionExport(FSGetStatFile);
   RegisterFunctionExport(FSGetStatFileAsync);
   RegisterFunctionExport(FSGetMountSource);
   RegisterFunctionExport(FSGetMountSourceAsync);
   RegisterFunctionExport(FSGetMountSourceNext);
   RegisterFunctionExport(FSGetMountSourceNextAsync);
   RegisterFunctionExport(FSIsEof);
   RegisterFunctionExport(FSIsEofAsync);
   RegisterFunctionExport(FSMakeDir);
   RegisterFunctionExport(FSMakeDirAsync);
   RegisterFunctionExport(FSMount);
   RegisterFunctionExport(FSMountAsync);
   RegisterFunctionExport(FSOpenDir);
   RegisterFunctionExport(FSOpenDirAsync);
   RegisterFunctionExport(FSOpenFile);
   RegisterFunctionExport(FSOpenFileAsync);
   RegisterFunctionExport(FSOpenFileEx);
   RegisterFunctionExport(FSOpenFileExAsync);
   RegisterFunctionExport(FSReadDir);
   RegisterFunctionExport(FSReadDirAsync);
   RegisterFunctionExport(FSReadFile);
   RegisterFunctionExport(FSReadFileAsync);
   RegisterFunctionExport(FSReadFileWithPos);
   RegisterFunctionExport(FSReadFileWithPosAsync);
   RegisterFunctionExport(FSRemove);
   RegisterFunctionExport(FSRemoveAsync);
   RegisterFunctionExport(FSRename);
   RegisterFunctionExport(FSRenameAsync);
   RegisterFunctionExport(FSRewindDir);
   RegisterFunctionExport(FSRewindDirAsync);
   RegisterFunctionExport(FSSetPosFile);
   RegisterFunctionExport(FSSetPosFileAsync);
   RegisterFunctionExport(FSTruncateFile);
   RegisterFunctionExport(FSTruncateFileAsync);
   RegisterFunctionExport(FSUnmount);
   RegisterFunctionExport(FSUnmountAsync);
   RegisterFunctionExport(FSWriteFile);
   RegisterFunctionExport(FSWriteFileAsync);
   RegisterFunctionExport(FSWriteFileWithPos);
   RegisterFunctionExport(FSWriteFileWithPosAsync);
}

} // namespace cafe::coreinit
