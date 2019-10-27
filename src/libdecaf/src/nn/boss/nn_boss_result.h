#pragma once
#include "nn/nn_result.h"

namespace nn::boss
{

static constexpr Result ResultSuccess {
   Result::MODULE_NN_BOSS, Result::LEVEL_SUCCESS, 0x00080
};

static constexpr Result ResultNOT_INITIALIZED {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 0x03200
};

static constexpr Result ResultLIBRARY_NOT_INITIALIZED {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 0x03280
};

static constexpr Result ResultINVALID {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 0x03700
};

static constexpr Result ResultInvalidParameter {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 0x03780
};

static constexpr Result ResultINVALID_FORMAT {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 0x03800
};

static constexpr Result ResultINVALID_ACCOUNT {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 0x03880
};

static constexpr Result ResultINVALID_TITLE {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 0x03900
};

static constexpr Result ResultNO_SUPPORT {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 0x04100
};

static constexpr Result ResultINITIALIZED {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x1f400
};

static constexpr Result ResultNOT_EXIST {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x1f900
};

static constexpr Result ResultFILE_NOT_EXIST {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x1f980
};

static constexpr Result ResultBOSS_STORAGE_NOT_EXIST {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x1fa00
};

static constexpr Result ResultDB_NOT_EXIST {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x1fa80
};

static constexpr Result ResultRECORD_NOT_EXIST {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x1fb00
};

static constexpr Result ResultNOT_COMPLETED {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x20300
};

static constexpr Result ResultNOT_PERMITTED {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x20800
};

static constexpr Result ResultFULL {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x20d00
};

static constexpr Result ResultSIZE_FULL {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x20d80
};

static constexpr Result ResultCOUNT_FULL {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x20e00
};

static constexpr Result ResultFINISHED {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x21200
};

static constexpr Result ResultSERVICE_FINISHED {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x21280
};

static constexpr Result ResultCANCELED {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x21700
};

static constexpr Result ResultSTOPPED_BY_POLICYLIST {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x21c00
};

static constexpr Result ResultALREADY_EXIST {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x22100
};

static constexpr Result ResultCANNOT_GET_NETWORK_TIME {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x22600
};

static constexpr Result ResultNOT_NETWORK_ACCOUNT {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x22b00
};

static constexpr Result ResultRESTRICTED_BY_PARENTAL_CONTROL {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x23000
};

static constexpr Result ResultDISABLE_UPLOAD_CONSOLE_INFORMATION {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x23500
};

static constexpr Result ResultNOT_CONNECT_NETWORK {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x23a00
};

static constexpr Result ResultRESTRICTED_BY_PARENTAL_CONTROL_TOTAL_ENABLE {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x23f00
};

static constexpr Result ResultNOT_FOUND {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x24400
};

static constexpr Result ResultBOSS_STORAGE_NOT_FOUND {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x24480
};

static constexpr Result ResultHTTP_ERROR {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x3e800
};

static constexpr Result ResultFS_ERROR {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4E200
};

static constexpr Result ResultFS_ERROR_NOT_INIT {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e280
};

static constexpr Result ResultFS_ERROR_BUSY {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e300
};

static constexpr Result ResultFS_ERROR_CANCELED {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e380
};

static constexpr Result ResultFS_ERROR_END_OF_DIRECTORY {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e400
};

static constexpr Result ResultFS_ERROR_END_OF_FILE {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e480
};

static constexpr Result ResultFS_ERROR_MAX_MOUNTPOINTS {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e500
};

static constexpr Result ResultFS_ERROR_MAX_VOLUMES {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e580
};

static constexpr Result ResultFS_ERROR_MAX_CLIENTS {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e600
};

static constexpr Result ResultFS_ERROR_MAX_FILES {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e680
};

static constexpr Result ResultFS_ERROR_MAX_DIRS {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e700
};

static constexpr Result ResultFS_ERROR_ALREADY_OPEN {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e780
};

static constexpr Result ResultFS_ERROR_ALREADY_EXISTS {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e800
};

static constexpr Result ResultFS_ERROR_NOT_FOUND {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e880
};

static constexpr Result ResultFS_ERROR_NOT_EMPTY {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e900
};

static constexpr Result ResultFS_ERROR_ACCESS_ERROR {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e980
};

static constexpr Result ResultFS_ERROR_PERMISSION_ERROR {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ea00
};

static constexpr Result ResultFS_ERROR_DATA_CORRUPTED {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ea80
};

static constexpr Result ResultFS_ERROR_STORAGE_FULL {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4eb00
};

static constexpr Result ResultFS_ERROR_JOURNAL_FULL {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4eb80
};

static constexpr Result ResultFS_ERROR_UNAVAILABLE_CMD {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ec00
};

static constexpr Result ResultFS_ERROR_UNSUPPORTED_CMD {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ec80
};

static constexpr Result ResultFS_ERROR_INVALID_PARAM {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ed00
};

static constexpr Result ResultFS_ERROR_INVALID_PATH {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ed80
};

static constexpr Result ResultFS_ERROR_INVALID_BUFFER {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ee00
};

static constexpr Result ResultFS_ERROR_INVALID_ALIGNMENT {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ee80
};

static constexpr Result ResultFS_ERROR_INVALID_CLIENT_HANDLE {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ef00
};

static constexpr Result ResultFS_ERROR_INVALID_FILE_HANDLE {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ef80
};

static constexpr Result ResultFS_ERROR_INVALID_DIR_HANDLE {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f000
};

static constexpr Result ResultFS_ERROR_NOT_FILE {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f080
};

static constexpr Result ResultFS_ERROR_NOT_DIR {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f100
};

static constexpr Result ResultFS_ERROR_FILE_TOO_BIG {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f180
};

static constexpr Result ResultFS_ERROR_OUT_OF_RANGE {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f200
};

static constexpr Result ResultFS_ERROR_OUT_OF_RESOURCES {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f280
};

static constexpr Result ResultFS_ERROR_MEDIA_NOT_READY {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f300
};

static constexpr Result ResultFS_ERROR_MEDIA_ERROR {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f380
};

static constexpr Result ResultFS_ERROR_WRITE_PROTECTED {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f400
};

static constexpr Result ResultFS_ERROR_UNKNOWN {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f480
};

static constexpr Result ResultFAIL {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5aa00
};

static constexpr Result ResultMEMORY_ALLOCATE_ERROR {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5dc00
};

static constexpr Result ResultINITIALIZE_ERROR {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e100
};

static constexpr Result ResultSSL_INITIALIZE_ERROR {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e180
};

static constexpr Result ResultACP_INITIALIZE_ERROR {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e200
};

static constexpr Result ResultACT_INITIALIZE_ERROR {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e280
};

static constexpr Result ResultPDM_INITIALIZE_ERROR {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e300
};

static constexpr Result ResultCONFIG_INITIALIZE_ERROR {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e380
};

static constexpr Result ResultFS_INITIALIZE_ERROR {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e400
};

static constexpr Result ResultHTTP_INITIALIZE_ERROR {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e480
};

static constexpr Result ResultAC_INITIALIZE_ERROR {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e500
};

static constexpr Result ResultUNEXPECT {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x7ff80
};

} // namespace nn::boss
