#pragma once
#include "nn/nn_result.h"

namespace nn::act
{

static constexpr Result ResultMailAddressNotConfirmed {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_USAGE, 0x80
};


static constexpr Result ResultLibraryError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_USAGE, 0xFA00
};

static constexpr Result ResultNotInitialised {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_USAGE, 0xFA80
};

static constexpr Result ResultAlreadyInitialised {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_USAGE, 0xFB00
};

static constexpr Result ResultBusy {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_USAGE, 0xFF80
};


static constexpr Result ResultNotImplemented {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_USAGE, 0x12780
};

static constexpr Result ResultDeprecated {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_USAGE, 0x12800
};

static constexpr Result ResultDevelopmentOnly {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_USAGE, 0x12880
};

static constexpr Result ResultInvalidArgument {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_USAGE, 0x12C00
};

static constexpr Result ResultInvalidPointer {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_USAGE, 0x12C80
};

static constexpr Result ResultOutOfRange {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_USAGE, 0x12D00
};

static constexpr Result ResultInvalidSize {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_USAGE, 0x12D80
};

static constexpr Result ResultInvalidFormat {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_USAGE, 0x12E00
};

static constexpr Result ResultInvalidHandle {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_USAGE, 0x12E80
};

static constexpr Result ResultInvalidValue {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_USAGE, 0x12F00
};


static constexpr Result ResultInternalError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x15E00
};

static constexpr Result ResultEndOfStream {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x15E80
};


static constexpr Result ResultFileError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x16300
};

static constexpr Result ResultFileNotFound {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_USAGE, 0x16380
};

static constexpr Result ResultFileVersionMismatch {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x16400
};

static constexpr Result ResultFileIoError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_FATAL, 0x16480
};

static constexpr Result ResultFileTypeMismatch {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x16500
};


static constexpr Result ResultOutOfResource {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x16D00
};

static constexpr Result ResultShortOfBuffer {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x16D80
};

static constexpr Result ResultOutOfMemory {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x17200
};

static constexpr Result ResultOutOfGlobalHeap {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x17280
};

static constexpr Result ResultOutOfCrossProcessHeap {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x17300
};

static constexpr Result ResultOutOfProcessLocalHeap {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x17380
};

static constexpr Result ResultOutOfMxmlHeap {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x17400
};


static constexpr Result ResultUcError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x19000
};

static constexpr Result ResultUcReadSysConfigError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x19080
};


static constexpr Result ResultMcpError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x19500
};

static constexpr Result ResultMcpOpenError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x19580
};

static constexpr Result ResultMcpGetInfoError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x19600
};


static constexpr Result ResultIsoError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x19A00
};

static constexpr Result ResultIsoInitFailure {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x19A80
};

static constexpr Result ResultIsoGetCountryCodeFailure {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x19B00
};

static constexpr Result ResultIsoGetLanguageCodeFailure {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x19B80
};

static constexpr Result ResultMXML_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1a900
};

static constexpr Result ResultIOS_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1c200
};

static constexpr Result ResultIOS_OPEN_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1c280
};

static constexpr Result ResultACCOUNT_MANAGEMENT_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1f400
};

static constexpr Result ResultACCOUNT_NOT_FOUND {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1f480
};

static constexpr Result ResultSLOTS_FULL {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1f500
};

static constexpr Result ResultACCOUNT_NOT_LOADED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1f980
};

static constexpr Result ResultACCOUNT_ALREADY_LOADED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1fa00
};

static constexpr Result ResultACCOUNT_LOCKED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1fa80
};

static constexpr Result ResultNOT_NETWORK_ACCOUNT {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1fe80
};

static constexpr Result ResultNOT_LOCAL_ACCOUNT {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1ff00
};

static constexpr Result ResultACCOUNT_NOT_COMMITTED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1ff80
};

static constexpr Result ResultNETWORK_CLOCK_INVALID {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x22680
};

static constexpr Result ResultAUTHENTICATION_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x3e800
};

static constexpr Result ResultHTTP_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41a00
};

static constexpr Result ResultHTTP_UNSUPPORTED_PROTOCOL {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41a80
};

static constexpr Result ResultHTTP_FAILED_INIT {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41b00
};

static constexpr Result ResultHTTP_URL_MALFORMAT {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41b80
};

static constexpr Result ResultHTTP_NOT_BUILT_IN {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41c00
};

static constexpr Result ResultHTTP_COULDNT_RESOLVE_PROXY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41c80
};

static constexpr Result ResultHTTP_COULDNT_RESOLVE_HOST {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41d00
};

static constexpr Result ResultHTTP_COULDNT_CONNECT {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41d80
};

static constexpr Result ResultHTTP_FTP_WEIRD_SERVER_REPLY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41e00
};

static constexpr Result ResultHTTP_REMOTE_ACCESS_DENIED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41e80
};

static constexpr Result ResultHTTP_OBSOLETE10 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41f00
};

static constexpr Result ResultHTTP_FTP_WEIRD_PASS_REPLY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41f80
};

static constexpr Result ResultHTTP_OBSOLETE12 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42000
};

static constexpr Result ResultHTTP_FTP_WEIRD_PASV_REPLY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42080
};

static constexpr Result ResultHTTP_FTP_WEIRD_227_FORMAT {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42100
};

static constexpr Result ResultHTTP_FTP_CANT_GET_HOST {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42180
};

static constexpr Result ResultHTTP_OBSOLETE16 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42200
};

static constexpr Result ResultHTTP_FTP_COULDNT_SET_TYPE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42280
};

static constexpr Result ResultHTTP_PARTIAL_FILE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42300
};

static constexpr Result ResultHTTP_FTP_COULDNT_RETR_FILE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42380
};

static constexpr Result ResultHTTP_OBSOLETE20 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42400
};

static constexpr Result ResultHTTP_QUOTE_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42480
};

static constexpr Result ResultHTTP_HTTP_RETURNED_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42500
};

static constexpr Result ResultHTTP_WRITE_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42580
};

static constexpr Result ResultHTTP_OBSOLETE24 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42600
};

static constexpr Result ResultHTTP_UPLOAD_FAILED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42680
};

static constexpr Result ResultHTTP_READ_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42700
};

static constexpr Result ResultHTTP_OUT_OF_MEMORY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42780
};

static constexpr Result ResultHTTP_OPERATION_TIMEDOUT {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42800
};

static constexpr Result ResultHTTP_OBSOLETE29 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42880
};

static constexpr Result ResultHTTP_FTP_PORT_FAILED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42900
};

static constexpr Result ResultHTTP_FTP_COULDNT_USE_REST {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42980
};

static constexpr Result ResultHTTP_OBSOLETE32 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42a00
};

static constexpr Result ResultHTTP_RANGE_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42a80
};

static constexpr Result ResultHTTP_HTTP_POST_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42b00
};

static constexpr Result ResultHTTP_SSL_CONNECT_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42b80
};

static constexpr Result ResultHTTP_BAD_DOWNLOAD_RESUME {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42c00
};

static constexpr Result ResultHTTP_FILE_COULDNT_READ_FILE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42c80
};

static constexpr Result ResultHTTP_LDAP_CANNOT_BIND {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42d00
};

static constexpr Result ResultHTTP_LDAP_SEARCH_FAILED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42d80
};

static constexpr Result ResultHTTP_OBSOLETE40 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42e00
};

static constexpr Result ResultHTTP_FUNCTION_NOT_FOUND {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42e80
};

static constexpr Result ResultHTTP_ABORTED_BY_CALLBACK {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42f00
};

static constexpr Result ResultHTTP_BAD_FUNCTION_ARGUMENT {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42f80
};

static constexpr Result ResultHTTP_OBSOLETE44 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43000
};

static constexpr Result ResultHTTP_INTERFACE_FAILED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43080
};

static constexpr Result ResultHTTP_OBSOLETE46 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43100
};

static constexpr Result ResultHTTP_TOO_MANY_REDIRECTS {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43180
};

static constexpr Result ResultHTTP_UNKNOWN_OPTION {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43200
};

static constexpr Result ResultHTTP_TELNET_OPTION_SYNTAX {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43280
};

static constexpr Result ResultHTTP_OBSOLETE50 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43300
};

static constexpr Result ResultHTTP_PEER_FAILED_VERIFICATION {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43380
};

static constexpr Result ResultHTTP_GOT_NOTHING {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43400
};

static constexpr Result ResultHTTP_SSL_ENGINE_NOTFOUND {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43480
};

static constexpr Result ResultHTTP_SSL_ENGINE_SETFAILED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43500
};

static constexpr Result ResultHTTP_SEND_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43580
};

static constexpr Result ResultHTTP_RECV_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43600
};

static constexpr Result ResultHTTP_OBSOLETE57 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43680
};

static constexpr Result ResultHTTP_SSL_CERTPROBLEM {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43700
};

static constexpr Result ResultHTTP_SSL_CIPHER {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43780
};

static constexpr Result ResultHTTP_SSL_CACERT {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43800
};

static constexpr Result ResultHTTP_BAD_CONTENT_ENCODING {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43880
};

static constexpr Result ResultHTTP_LDAP_INVALID_URL {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43900
};

static constexpr Result ResultHTTP_FILESIZE_EXCEEDED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43980
};

static constexpr Result ResultHTTP_USE_SSL_FAILED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43a00
};

static constexpr Result ResultHTTP_SEND_FAIL_REWIND {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43a80
};

static constexpr Result ResultHTTP_SSL_ENGINE_INITFAILED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43b00
};

static constexpr Result ResultHTTP_LOGIN_DENIED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43b80
};

static constexpr Result ResultHTTP_TFTP_NOTFOUND {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43c00
};

static constexpr Result ResultHTTP_TFTP_PERM {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43c80
};

static constexpr Result ResultHTTP_REMOTE_DISK_FULL {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43d00
};

static constexpr Result ResultHTTP_TFTP_ILLEGAL {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43d80
};

static constexpr Result ResultHTTP_TFTP_UNKNOWNID {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43e00
};

static constexpr Result ResultHTTP_REMOTE_FILE_EXISTS {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43e80
};

static constexpr Result ResultHTTP_TFTP_NOSUCHUSER {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43f00
};

static constexpr Result ResultHTTP_CONV_FAILED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43f80
};

static constexpr Result ResultHTTP_CONV_REQD {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44000
};

static constexpr Result ResultHTTP_SSL_CACERT_BADFILE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44080
};

static constexpr Result ResultHTTP_REMOTE_FILE_NOT_FOUND {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44100
};

static constexpr Result ResultHTTP_SSH {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44180
};

static constexpr Result ResultHTTP_SSL_SHUTDOWN_FAILED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44200
};

static constexpr Result ResultHTTP_AGAIN {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44280
};

static constexpr Result ResultHTTP_SSL_CRL_BADFILE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44300
};

static constexpr Result ResultHTTP_SSL_ISSUER_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44380
};

static constexpr Result ResultHTTP_FTP_PRET_FAILED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44400
};

static constexpr Result ResultHTTP_RTSP_CSEQ_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44480
};

static constexpr Result ResultHTTP_RTSP_SESSION_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44500
};

static constexpr Result ResultHTTP_FTP_BAD_FILE_LIST {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44580
};

static constexpr Result ResultHTTP_CHUNK_FAILED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44600
};

static constexpr Result ResultHTTP_NSSL_NO_CTX {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44680
};

static constexpr Result ResultSO_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x45100
};

static constexpr Result ResultSO_SELECT_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x45180
};

static constexpr Result ResultREQUEST_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4b000
};

static constexpr Result ResultBAD_FORMAT_PARAMETER {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4b080
};

static constexpr Result ResultBAD_FORMAT_REQUEST {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4b100
};

static constexpr Result ResultREQUEST_PARAMETER_MISSING {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4b180
};

static constexpr Result ResultWRONG_HTTP_METHOD {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4b200
};

static constexpr Result ResultRESPONSE_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4ba00
};

static constexpr Result ResultBAD_FORMAT_RESPONSE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4ba80
};

static constexpr Result ResultRESPONSE_ITEM_MISSING {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4bb00
};

static constexpr Result ResultRESPONSE_TOO_LARGE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4bb80
};

static constexpr Result ResultNOT_MODIFIED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4c480
};

static constexpr Result ResultINVALID_COMMON_PARAMETER {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4c900
};

static constexpr Result ResultINVALID_PLATFORM_ID {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4c980
};

static constexpr Result ResultUNAUTHORIZED_DEVICE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4ca00
};

static constexpr Result ResultINVALID_SERIAL_ID {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4ca80
};

static constexpr Result ResultINVALID_MAC_ADDRESS {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4cb00
};

static constexpr Result ResultINVALID_REGION {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4cb80
};

static constexpr Result ResultINVALID_COUNTRY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4cc00
};

static constexpr Result ResultINVALID_LANGUAGE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4cc80
};

static constexpr Result ResultUNAUTHORIZED_CLIENT {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4cd00
};

static constexpr Result ResultDEVICE_ID_EMPTY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4cd80
};

static constexpr Result ResultSERIAL_ID_EMPTY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4ce00
};

static constexpr Result ResultPLATFORM_ID_EMPTY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4ce80
};

static constexpr Result ResultINVALID_UNIQUE_ID {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4d380
};

static constexpr Result ResultINVALID_CLIENT_ID {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4d400
};

static constexpr Result ResultINVALID_CLIENT_KEY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4d480
};

static constexpr Result ResultINVALID_NEX_CLIENT_ID {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4d880
};

static constexpr Result ResultINVALID_GAME_SERVER_ID {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4d900
};

static constexpr Result ResultGAME_SERVER_ID_ENVIRONMENT_NOT_FOUND {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4d980
};

static constexpr Result ResultGAME_SERVER_ID_UNIQUE_ID_NOT_LINKED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4da00
};

static constexpr Result ResultCLIENT_ID_UNIQUE_ID_NOT_LINKED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4da80
};

static constexpr Result ResultDEVICE_MISMATCH {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4e280
};

static constexpr Result ResultCOUNTRY_MISMATCH {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4e300
};

static constexpr Result ResultEULA_NOT_ACCEPTED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4e380
};

static constexpr Result ResultUPDATE_REQUIRED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4e700
};

static constexpr Result ResultSYSTEM_UPDATE_REQUIRED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4e780
};

static constexpr Result ResultAPPLICATION_UPDATE_REQUIRED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4e800
};

static constexpr Result ResultUNAUTHORIZED_REQUEST {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4ec00
};

static constexpr Result ResultREQUEST_FORBIDDEN {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4ed00
};

static constexpr Result ResultRESOURCE_NOT_FOUND {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f100
};

static constexpr Result ResultPID_NOT_FOUND {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f180
};

static constexpr Result ResultNEX_ACCOUNT_NOT_FOUND {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f200
};

static constexpr Result ResultGENERATE_TOKEN_FAILURE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f280
};

static constexpr Result ResultREQUEST_NOT_FOUND {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f300
};

static constexpr Result ResultMASTER_PIN_NOT_FOUND {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f380
};

static constexpr Result ResultMAIL_TEXT_NOT_FOUND {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f400
};

static constexpr Result ResultSEND_MAIL_FAILURE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f480
};

static constexpr Result ResultAPPROVAL_ID_NOT_FOUND {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f500
};

static constexpr Result ResultINVALID_EULA_PARAMETER {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f600
};

static constexpr Result ResultINVALID_EULA_COUNTRY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f680
};

static constexpr Result ResultINVALID_EULA_COUNTRY_AND_VERSION {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f700
};

static constexpr Result ResultEULA_NOT_FOUND {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f780
};

static constexpr Result ResultPHRASE_NOT_ACCEPTABLE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50500
};

static constexpr Result ResultACCOUNT_ID_ALREADY_EXISTS {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50580
};

static constexpr Result ResultACCOUNT_ID_NOT_ACCEPTABLE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50600
};

static constexpr Result ResultACCOUNT_PASSWORD_NOT_ACCEPTABLE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50680
};

static constexpr Result ResultMII_NAME_NOT_ACCEPTABLE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50700
};

static constexpr Result ResultMAIL_ADDRESS_NOT_ACCEPTABLE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50780
};

static constexpr Result ResultACCOUNT_ID_FORMAT_INVALID {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50800
};

static constexpr Result ResultACCOUNT_ID_PASSWORD_SAME {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50880
};

static constexpr Result ResultACCOUNT_ID_CHAR_NOT_ACCEPTABLE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50900
};

static constexpr Result ResultACCOUNT_ID_SUCCESSIVE_SYMBOL {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50980
};

static constexpr Result ResultACCOUNT_ID_SYMBOL_POSITION_NOT_ACCEPTABLE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50a00
};

static constexpr Result ResultACCOUNT_ID_TOO_MANY_DIGIT {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50a80
};

static constexpr Result ResultACCOUNT_PASSWORD_CHAR_NOT_ACCEPTABLE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50b00
};

static constexpr Result ResultACCOUNT_PASSWORD_TOO_FEW_CHAR_TYPES {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50b80
};

static constexpr Result ResultACCOUNT_PASSWORD_SUCCESSIVE_SAME_CHAR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50c00
};

static constexpr Result ResultMAIL_ADDRESS_DOMAIN_NAME_NOT_ACCEPTABLE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50c80
};

static constexpr Result ResultMAIL_ADDRESS_DOMAIN_NAME_NOT_RESOLVED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50d00
};

static constexpr Result ResultREACHED_ASSOCIATION_LIMIT {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50f80
};

static constexpr Result ResultREACHED_REGISTRATION_LIMIT {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51000
};

static constexpr Result ResultCOPPA_NOT_ACCEPTED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51080
};

static constexpr Result ResultPARENTAL_CONTROLS_REQUIRED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51100
};

static constexpr Result ResultMII_NOT_REGISTERED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51180
};

static constexpr Result ResultDEVICE_EULA_COUNTRY_MISMATCH {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51200
};

static constexpr Result ResultPENDING_MIGRATION {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51280
};

static constexpr Result ResultWRONG_USER_INPUT {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51900
};

static constexpr Result ResultWRONG_ACCOUNT_PASSWORD {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51980
};

static constexpr Result ResultWRONG_MAIL_ADDRESS {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51a00
};

static constexpr Result ResultWRONG_ACCOUNT_PASSWORD_OR_MAIL_ADDRESS {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51a80
};

static constexpr Result ResultWRONG_CONFIRMATION_CODE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51b00
};

static constexpr Result ResultWRONG_BIRTH_DATE_OR_MAIL_ADDRESS {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51b80
};

static constexpr Result ResultWRONG_ACCOUNT_MAIL {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51c00
};

static constexpr Result ResultACCOUNT_ALREADY_DELETED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52380
};

static constexpr Result ResultACCOUNT_ID_CHANGED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52400
};

static constexpr Result ResultAUTHENTICATION_LOCKED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52480
};

static constexpr Result ResultDEVICE_INACTIVE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52500
};

static constexpr Result ResultCOPPA_AGREEMENT_CANCELED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52580
};

static constexpr Result ResultDOMAIN_ACCOUNT_ALREADY_EXISTS {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52600
};

static constexpr Result ResultACCOUNT_TOKEN_EXPIRED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52880
};

static constexpr Result ResultINVALID_ACCOUNT_TOKEN {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52900
};

static constexpr Result ResultAUTHENTICATION_REQUIRED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52980
};

static constexpr Result ResultCONFIRMATION_CODE_EXPIRED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52d80
};

static constexpr Result ResultMAIL_ADDRESS_NOT_VALIDATED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53280
};

static constexpr Result ResultEXCESSIVE_MAIL_SEND_REQUEST {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53300
};

static constexpr Result ResultCREDIT_CARD_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53700
};

static constexpr Result ResultCREDIT_CARD_GENERAL_FAILURE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53780
};

static constexpr Result ResultCREDIT_CARD_DECLINED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53800
};

static constexpr Result ResultCREDIT_CARD_BLACKLISTED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53880
};

static constexpr Result ResultINVALID_CREDIT_CARD_NUMBER {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53900
};

static constexpr Result ResultINVALID_CREDIT_CARD_DATE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53980
};

static constexpr Result ResultINVALID_CREDIT_CARD_PIN {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53a00
};

static constexpr Result ResultINVALID_POSTAL_CODE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53a80
};

static constexpr Result ResultINVALID_LOCATION {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53b00
};

static constexpr Result ResultCREDIT_CARD_DATE_EXPIRED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53b80
};

static constexpr Result ResultCREDIT_CARD_NUMBER_WRONG {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53c00
};

static constexpr Result ResultCREDIT_CARD_PIN_WRONG {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53c80
};

static constexpr Result ResultBANNED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57800
};

static constexpr Result ResultBANNED_ACCOUNT {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57880
};

static constexpr Result ResultBANNED_ACCOUNT_ALL {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57900
};

static constexpr Result ResultBANNED_ACCOUNT_IN_APPLICATION {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57980
};

static constexpr Result ResultBANNED_ACCOUNT_IN_NEX_SERVICE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57a00
};

static constexpr Result ResultBANNED_ACCOUNT_IN_INDEPENDENT_SERVICE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57a80
};

static constexpr Result ResultBANNED_DEVICE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57d80
};

static constexpr Result ResultBANNED_DEVICE_ALL {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57e00
};

static constexpr Result ResultBANNED_DEVICE_IN_APPLICATION {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57e80
};

static constexpr Result ResultBANNED_DEVICE_IN_NEX_SERVICE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57f00
};

static constexpr Result ResultBANNED_DEVICE_IN_INDEPENDENT_SERVICE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57f80
};

static constexpr Result ResultBANNED_ACCOUNT_TEMPORARILY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58280
};

static constexpr Result ResultBANNED_ACCOUNT_ALL_TEMPORARILY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58300
};

static constexpr Result ResultBANNED_ACCOUNT_IN_APPLICATION_TEMPORARILY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58380
};

static constexpr Result ResultBANNED_ACCOUNT_IN_NEX_SERVICE_TEMPORARILY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58400
};

static constexpr Result ResultBANNED_ACCOUNT_IN_INDEPENDENT_SERVICE_TEMPORARILY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58480
};

static constexpr Result ResultBANNED_DEVICE_TEMPORARILY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58780
};

static constexpr Result ResultBANNED_DEVICE_ALL_TEMPORARILY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58800
};

static constexpr Result ResultBANNED_DEVICE_IN_APPLICATION_TEMPORARILY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58880
};

static constexpr Result ResultBANNED_DEVICE_IN_NEX_SERVICE_TEMPORARILY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58900
};

static constexpr Result ResultBANNED_DEVICE_IN_INDEPENDENT_SERVICE_TEMPORARILY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58980
};

static constexpr Result ResultSERVICE_NOT_PROVIDED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5a000
};

static constexpr Result ResultUNDER_MAINTENANCE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5a080
};

static constexpr Result ResultSERVICE_CLOSED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5a100
};

static constexpr Result ResultNINTENDO_NETWORK_CLOSED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5a180
};

static constexpr Result ResultNOT_PROVIDED_COUNTRY {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5a200
};

static constexpr Result ResultRESTRICTION_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5aa00
};

static constexpr Result ResultRESTRICTED_BY_AGE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5aa80
};

static constexpr Result ResultRESTRICTED_BY_PARENTAL_CONTROLS {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5af00
};

static constexpr Result ResultON_GAME_INTERNET_COMMUNICATION_RESTRICTED {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5af80
};

static constexpr Result ResultINTERNAL_SERVER_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5b980
};

static constexpr Result ResultUNKNOWN_SERVER_ERROR {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5ba00
};

static constexpr Result ResultUNAUTHENTICATED_AFTER_SALVAGE {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5db00
};

static constexpr Result ResultAUTHENTICATION_FAILURE_UNKNOWN {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5db80
};

} // namespace nn::act
