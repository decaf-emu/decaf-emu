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

static constexpr Result ResultMxmlError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1a900
};

static constexpr Result ResultIosError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1c200
};

static constexpr Result ResultIosOpenError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1c280
};

static constexpr Result ResultAccountManagementError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1f400
};

static constexpr Result ResultAccountNotFound {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1f480
};

static constexpr Result ResultSlotsFull {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1f500
};

static constexpr Result ResultAccountNotLoaded {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1f980
};

static constexpr Result ResultAccountAlreadyLoaded {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1fa00
};

static constexpr Result ResultAccountLocked {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1fa80
};

static constexpr Result ResultNotNetworkAccount {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1fe80
};

static constexpr Result ResultNotLocalAccount {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1ff00
};

static constexpr Result ResultAccountNotCommitted {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x1ff80
};

static constexpr Result ResultNetworkClockInvalid {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x22680
};

static constexpr Result ResultAuthenticationError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x3e800
};

static constexpr Result ResultHttpError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41a00
};

static constexpr Result ResultHttpUnsupportedProtocol {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41a80
};

static constexpr Result ResultHttpFailedInit {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41b00
};

static constexpr Result ResultHttpURLMalformat {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41b80
};

static constexpr Result ResultHttpNotBuiltIn {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41c00
};

static constexpr Result ResultHttpCouldntResolveProxy {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41c80
};

static constexpr Result ResultHttpCouldntResolveHost {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41d00
};

static constexpr Result ResultHttpCouldntConnect {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41d80
};

static constexpr Result ResultHttpFtpWeirdServerReply {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41e00
};

static constexpr Result ResultHttpRemoteAccessDenied {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41e80
};

static constexpr Result ResultHttpObsolete10 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41f00
};

static constexpr Result ResultHttpFtpWeirdPassReply {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x41f80
};

static constexpr Result ResultHttpObsolete12 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42000
};

static constexpr Result ResultHttpFtpWeirdPasvReply {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42080
};

static constexpr Result ResultHttpFtpWeird227Format {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42100
};

static constexpr Result ResultHttpFtpCantGetHost {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42180
};

static constexpr Result ResultHttpObsolete16 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42200
};

static constexpr Result ResultHttpFtpCouldntSetType {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42280
};

static constexpr Result ResultHttpPartialFile {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42300
};

static constexpr Result ResultHttpFtpCouldntRetrFile {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42380
};

static constexpr Result ResultHttpObsolete20 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42400
};

static constexpr Result ResultHttpQuoteError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42480
};

static constexpr Result ResultHttpHttpReturnedError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42500
};

static constexpr Result ResultHttpWriteError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42580
};

static constexpr Result ResultHttpObsolete24 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42600
};

static constexpr Result ResultHttpUploadFailed {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42680
};

static constexpr Result ResultHttpReadError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42700
};

static constexpr Result ResultHttpOutOfMemory {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42780
};

static constexpr Result ResultHttpOperationTimedout {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42800
};

static constexpr Result ResultHttpObsolete29 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42880
};

static constexpr Result ResultHttpFtpPortFailed {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42900
};

static constexpr Result ResultHttpFtpCouldntUseRest {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42980
};

static constexpr Result ResultHttpObsolete32 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42a00
};

static constexpr Result ResultHttpRangeError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42a80
};

static constexpr Result ResultHttpHttpPostError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42b00
};

static constexpr Result ResultHttpSslConnectError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42b80
};

static constexpr Result ResultHttpBadDownloadResume {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42c00
};

static constexpr Result ResultHttpFileCouldntReadFile {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42c80
};

static constexpr Result ResultHttpLdapCannotBind {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42d00
};

static constexpr Result ResultHttpLdapSearchFailed {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42d80
};

static constexpr Result ResultHttpObsolete40 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42e00
};

static constexpr Result ResultHttpFunctionNotFound {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42e80
};

static constexpr Result ResultHttpAbortedByCallback {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42f00
};

static constexpr Result ResultHttpBadFunctionArgument {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x42f80
};

static constexpr Result ResultHttpObsolete44 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43000
};

static constexpr Result ResultHttpInterfaceFailed {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43080
};

static constexpr Result ResultHttpObsolete46 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43100
};

static constexpr Result ResultHttpTooManyRedirects {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43180
};

static constexpr Result ResultHttpUnknownOption {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43200
};

static constexpr Result ResultHttpTelnetOptionSyntax {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43280
};

static constexpr Result ResultHttpObsolete50 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43300
};

static constexpr Result ResultHttpPeerFailedVerification {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43380
};

static constexpr Result ResultHttpGotNothing {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43400
};

static constexpr Result ResultHttpSslEngineNotfound {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43480
};

static constexpr Result ResultHttpSslEngineSetfailed {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43500
};

static constexpr Result ResultHttpSendError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43580
};

static constexpr Result ResultHttpRecvError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43600
};

static constexpr Result ResultHttpObsolete57 {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43680
};

static constexpr Result ResultHttpSslCertproblem {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43700
};

static constexpr Result ResultHttpSslCipher {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43780
};

static constexpr Result ResultHttpSslCacert {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43800
};

static constexpr Result ResultHttpBadContentEncoding {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43880
};

static constexpr Result ResultHttpLdapInvalidURL {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43900
};

static constexpr Result ResultHttpFilesizeExceeded {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43980
};

static constexpr Result ResultHttpUseSslFailed {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43a00
};

static constexpr Result ResultHttpSendFailRewind {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43a80
};

static constexpr Result ResultHttpSslEngineInitfailed {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43b00
};

static constexpr Result ResultHttpLoginDenied {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43b80
};

static constexpr Result ResultHttpTftpNotfound {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43c00
};

static constexpr Result ResultHttpTftpPerm {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43c80
};

static constexpr Result ResultHttpRemoteDiskFull {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43d00
};

static constexpr Result ResultHttpTftpIllegal {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43d80
};

static constexpr Result ResultHttpTftpUnknownid {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43e00
};

static constexpr Result ResultHttpRemoteFileExists {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43e80
};

static constexpr Result ResultHttpTftpNosuchuser {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43f00
};

static constexpr Result ResultHttpConvFailed {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x43f80
};

static constexpr Result ResultHttpConvReqd {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44000
};

static constexpr Result ResultHttpSslCacertBadfile {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44080
};

static constexpr Result ResultHttpRemoteFileNotFound {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44100
};

static constexpr Result ResultHttpSsh {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44180
};

static constexpr Result ResultHttpSslShutdownFailed {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44200
};

static constexpr Result ResultHttpAgain {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44280
};

static constexpr Result ResultHttpSslCrlBadfile {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44300
};

static constexpr Result ResultHttpSslIssuerError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44380
};

static constexpr Result ResultHttpFtpPretFailed {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44400
};

static constexpr Result ResultHttpRtspCseqError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44480
};

static constexpr Result ResultHttpRtspSessionError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44500
};

static constexpr Result ResultHttpFtpBadFileList {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44580
};

static constexpr Result ResultHttpChunkFailed {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44600
};

static constexpr Result ResultHttpNsslNoCtx {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x44680
};

static constexpr Result ResultSoError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x45100
};

static constexpr Result ResultSoSelectError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x45180
};

static constexpr Result ResultRequestError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4b000
};

static constexpr Result ResultBadFormatParameter {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4b080
};

static constexpr Result ResultBadFormatRequest {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4b100
};

static constexpr Result ResultRequestParameterMissing {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4b180
};

static constexpr Result ResultWrongHttpMethod {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4b200
};

static constexpr Result ResultResponseError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4ba00
};

static constexpr Result ResultBadFormatResponse {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4ba80
};

static constexpr Result ResultResponseItemMissing {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4bb00
};

static constexpr Result ResultResponseTooLarge {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4bb80
};

static constexpr Result ResultNotModified {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4c480
};

static constexpr Result ResultInvalidCommonParameter {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4c900
};

static constexpr Result ResultInvalidPlatformID {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4c980
};

static constexpr Result ResultUnauthorizedDevice {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4ca00
};

static constexpr Result ResultInvalidSerialID {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4ca80
};

static constexpr Result ResultInvalidMacAddress {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4cb00
};

static constexpr Result ResultInvalidRegion {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4cb80
};

static constexpr Result ResultInvalidCountry {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4cc00
};

static constexpr Result ResultInvalidLanguage {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4cc80
};

static constexpr Result ResultUnauthorizedClient {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4cd00
};

static constexpr Result ResultDeviceIDEmpty {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4cd80
};

static constexpr Result ResultSerialIDEmpty {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4ce00
};

static constexpr Result ResultPlatformIDEmpty {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4ce80
};

static constexpr Result ResultInvalidUniqueID {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4d380
};

static constexpr Result ResultInvalidClientID {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4d400
};

static constexpr Result ResultInvalidClientKey {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4d480
};

static constexpr Result ResultInvalidNexClientID {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4d880
};

static constexpr Result ResultInvalidGameServerID {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4d900
};

static constexpr Result ResultGameServerIDEnvironmentNotFound {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4d980
};

static constexpr Result ResultGameServerIDUniqueIDNotLinked {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4da00
};

static constexpr Result ResultClientIDUniqueIDNotLinked {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4da80
};

static constexpr Result ResultDeviceMismatch {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4e280
};

static constexpr Result ResultCountryMismatch {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4e300
};

static constexpr Result ResultEulaNotAccepted {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4e380
};

static constexpr Result ResultUpdateRequired {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4e700
};

static constexpr Result ResultSystemUpdateRequired {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4e780
};

static constexpr Result ResultApplicationUpdateRequired {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4e800
};

static constexpr Result ResultUnauthorizedRequest {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4ec00
};

static constexpr Result ResultRequestForbidden {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4ed00
};

static constexpr Result ResultResourceNotFound {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f100
};

static constexpr Result ResultPidNotFound {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f180
};

static constexpr Result ResultNexAccountNotFound {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f200
};

static constexpr Result ResultGenerateTokenFailure {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f280
};

static constexpr Result ResultRequestNotFound {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f300
};

static constexpr Result ResultMasterPinNotFound {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f380
};

static constexpr Result ResultMailTextNotFound {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f400
};

static constexpr Result ResultSendMailFailure {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f480
};

static constexpr Result ResultApprovalIDNotFound {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f500
};

static constexpr Result ResultInvalidEulaParameter {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f600
};

static constexpr Result ResultInvalidEulaCountry {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f680
};

static constexpr Result ResultInvalidEulaCountryAndVersion {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f700
};

static constexpr Result ResultEulaNotFound {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x4f780
};

static constexpr Result ResultPhraseNotAcceptable {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50500
};

static constexpr Result ResultAccountIDAlreadyExists {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50580
};

static constexpr Result ResultAccountIDNotAcceptable {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50600
};

static constexpr Result ResultAccountPasswordNotAcceptable {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50680
};

static constexpr Result ResultMiiNameNotAcceptable {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50700
};

static constexpr Result ResultMailAddressNotAcceptable {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50780
};

static constexpr Result ResultAccountIDFormatInvalid {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50800
};

static constexpr Result ResultAccountIDPasswordSame {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50880
};

static constexpr Result ResultAccountIDCharNotAcceptable {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50900
};

static constexpr Result ResultAccountIDSuccessiveSymbol {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50980
};

static constexpr Result ResultAccountIDSymbolPositionNotAcceptable {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50a00
};

static constexpr Result ResultAccountIDTooManyDigit {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50a80
};

static constexpr Result ResultAccountPasswordCharNotAcceptable {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50b00
};

static constexpr Result ResultAccountPasswordTooFewCharTypes {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50b80
};

static constexpr Result ResultAccountPasswordSuccessiveSameChar {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50c00
};

static constexpr Result ResultMailAddressDomainNameNotAcceptable {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50c80
};

static constexpr Result ResultMailAddressDomainNameNotResolved {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50d00
};

static constexpr Result ResultReachedAssociationLimit {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x50f80
};

static constexpr Result ResultReachedRegistrationLimit {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51000
};

static constexpr Result ResultCoppaNotAccepted {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51080
};

static constexpr Result ResultParentalControlsRequired {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51100
};

static constexpr Result ResultMiiNotRegistered {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51180
};

static constexpr Result ResultDeviceEulaCountryMismatch {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51200
};

static constexpr Result ResultPendingMigration {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51280
};

static constexpr Result ResultWrongUserInput {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51900
};

static constexpr Result ResultWrongAccountPassword {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51980
};

static constexpr Result ResultWrongMailAddress {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51a00
};

static constexpr Result ResultWrongAccountPasswordOrMailAddress {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51a80
};

static constexpr Result ResultWrongConfirmationCode {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51b00
};

static constexpr Result ResultWrongBirthDateOrMailAddress {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51b80
};

static constexpr Result ResultWrongAccountMail {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x51c00
};

static constexpr Result ResultAccountAlreadyDeleted {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52380
};

static constexpr Result ResultAccountIDChanged {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52400
};

static constexpr Result ResultAuthenticationLocked {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52480
};

static constexpr Result ResultDeviceInactive {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52500
};

static constexpr Result ResultCoppaAgreementCanceled {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52580
};

static constexpr Result ResultDomainAccountAlreadyExists {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52600
};

static constexpr Result ResultAccountTokenExpired {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52880
};

static constexpr Result ResultInvalidAccountToken {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52900
};

static constexpr Result ResultAuthenticationRequired {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52980
};

static constexpr Result ResultConfirmationCodeExpired {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x52d80
};

static constexpr Result ResultMailAddressNotValidated {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53280
};

static constexpr Result ResultExcessiveMailSendRequest {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53300
};

static constexpr Result ResultCreditCardError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53700
};

static constexpr Result ResultCreditCardGeneralFailure {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53780
};

static constexpr Result ResultCreditCardDeclined {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53800
};

static constexpr Result ResultCreditCardBlacklisted {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53880
};

static constexpr Result ResultInvalidCreditCardNumber {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53900
};

static constexpr Result ResultInvalidCreditCardDate {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53980
};

static constexpr Result ResultInvalidCreditCardPin {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53a00
};

static constexpr Result ResultInvalidPostalCode {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53a80
};

static constexpr Result ResultInvalidLocation {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53b00
};

static constexpr Result ResultCreditCardDateExpired {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53b80
};

static constexpr Result ResultCreditCardNumberWrong {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53c00
};

static constexpr Result ResultCreditCardPinWrong {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x53c80
};

static constexpr Result ResultBanned {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57800
};

static constexpr Result ResultBannedAccount {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57880
};

static constexpr Result ResultBannedAccountAll {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57900
};

static constexpr Result ResultBannedAccountInApplication {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57980
};

static constexpr Result ResultBannedAccountInNexService {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57a00
};

static constexpr Result ResultBannedAccountInIndependentService {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57a80
};

static constexpr Result ResultBannedDevice {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57d80
};

static constexpr Result ResultBannedDeviceAll {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57e00
};

static constexpr Result ResultBannedDeviceInApplication {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57e80
};

static constexpr Result ResultBannedDeviceInNexService {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57f00
};

static constexpr Result ResultBannedDeviceInIndependentService {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x57f80
};

static constexpr Result ResultBannedAccountTemporarily {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58280
};

static constexpr Result ResultBannedAccountAllTemporarily {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58300
};

static constexpr Result ResultBannedAccountInApplicationTemporarily {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58380
};

static constexpr Result ResultBannedAccountInNexServiceTemporarily {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58400
};

static constexpr Result ResultBannedAccountInIndependentServiceTemporarily {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58480
};

static constexpr Result ResultBannedDeviceTemporarily {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58780
};

static constexpr Result ResultBannedDeviceAllTemporarily {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58800
};

static constexpr Result ResultBannedDeviceInApplicationTemporarily {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58880
};

static constexpr Result ResultBannedDeviceInNexServiceTemporarily {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58900
};

static constexpr Result ResultBannedDeviceInIndependentServiceTemporarily {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x58980
};

static constexpr Result ResultServiceNotProvided {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5a000
};

static constexpr Result ResultUnderMaintenance {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5a080
};

static constexpr Result ResultServiceClosed {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5a100
};

static constexpr Result ResultNintendoNetworkClosed {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5a180
};

static constexpr Result ResultNotProvidedCountry {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5a200
};

static constexpr Result ResultRestrictionError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5aa00
};

static constexpr Result ResultRestrictedByAge {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5aa80
};

static constexpr Result ResultRestrictedByParentalControls {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5af00
};

static constexpr Result ResultOnGameInternetCommunicationRestricted {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5af80
};

static constexpr Result ResultInternalServerError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5b980
};

static constexpr Result ResultUnknownServerError {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5ba00
};

static constexpr Result ResultUnauthenticatedAfterSalvage {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5db00
};

static constexpr Result ResultAuthenticationFailureUnknown {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 0x5db80
};

} // namespace nn::act
