#ifndef NN_ACT_ENUM_H
#define NN_ACT_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(nn)
ENUM_NAMESPACE_ENTER(act)

ENUM_BEG(ACTLoadOption, int32_t)
ENUM_END(ACTLoadOption)

ENUM_BEG(InfoType, int32_t)
   ENUM_VALUE(NumOfAccounts,                 1)
   ENUM_VALUE(SlotNo,                        2)
   ENUM_VALUE(DefaultAccount,                3)
   ENUM_VALUE(NetworkTimeDifference,         4)
   ENUM_VALUE(PersistentId,                  5)
   ENUM_VALUE(LocalFriendCode,               6)
   ENUM_VALUE(Mii,                           7)
   ENUM_VALUE(AccountId,                     8)
   ENUM_VALUE(EmailAddress,                  9)
   ENUM_VALUE(Birthday,                      10)
   ENUM_VALUE(Country,                       11)
   ENUM_VALUE(PrincipalId,                   12)
   ENUM_VALUE(IsPasswordCacheEnabled,        14)
   ENUM_VALUE(AccountPasswordCache,          15)
   ENUM_VALUE(AccountInfo,                   17)
   ENUM_VALUE(HostServerSettings,            18)
   ENUM_VALUE(Gender,                        19)
   ENUM_VALUE(LastAuthenticationResult,      20)
   ENUM_VALUE(StickyAccountId,               21)
   ENUM_VALUE(ParentalControlSlot,           22)
   ENUM_VALUE(SimpleAddressId,               23)
   ENUM_VALUE(UtcOffset,                     25)
   ENUM_VALUE(IsCommitted,                   26)
   ENUM_VALUE(MiiName,                       27)
   ENUM_VALUE(NfsPassword,                   28)
   ENUM_VALUE(HasEciVirtualAccount,          29)
   ENUM_VALUE(TimeZoneId,                    30)
   ENUM_VALUE(IsMiiUpdated,                  31)
   ENUM_VALUE(IsMailAddressValidated,        32)
   ENUM_VALUE(NextAccountId,                 33)
   ENUM_VALUE(Unk34,                         34)
   ENUM_VALUE(ApplicationUpdateRequired,     35)
   ENUM_VALUE(DefaultHostServerSettings,     36)
   ENUM_VALUE(IsServerAccountDeleted,        37)
   ENUM_VALUE(MiiImageUrl,                   38)
   ENUM_VALUE(StickyPrincipalId,             39)
   ENUM_VALUE(Unk40,                         40)
   ENUM_VALUE(Unk41,                         41)
   ENUM_VALUE(DefaultHostServerSettingsEx,   42)
   ENUM_VALUE(DeviceHash,                    43)
   ENUM_VALUE(ServerAccountStatus,           44)
   ENUM_VALUE(NetworkTime,                   46)
ENUM_END(InfoType)

ENUM_BEG(MiiImageType, int32_t)
ENUM_END(MiiImageType)

ENUM_NAMESPACE_EXIT(act)
ENUM_NAMESPACE_EXIT(nn)

#include <common/enum_end.inl>

#endif // ifdef NN_ACT_ENUM_H
