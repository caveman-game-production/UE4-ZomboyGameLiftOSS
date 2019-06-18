// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "OnlineIdentityZomboy.h"
#include "Misc/CommandLine.h"
#include "Misc/Guid.h"
#include "Misc/OutputDeviceRedirector.h"
#include "IPAddress.h"
#include "SocketSubsystem.h"
#include "OnlineError.h"
#include "OnlineSubsystemZomboy.h"
#include "OnlineSubsystemZomboyPrivate.h"
#include "Engine/Texture2DDynamic.h"
#include "Runtime/RenderCore/Public/RenderingThread.h"

#if WITH_OCULUS

#include "OnlineSessionAsyncOculus.h"

#endif

bool FUserOnlineAccountZomboy::GetAuthAttribute(const FString& AttrName, FString& OutAttrValue) const
{
	const FString* FoundAttr = AdditionalAuthData.Find(AttrName);
	if (FoundAttr != NULL)
	{
		OutAttrValue = *FoundAttr;
		return true;
	}
	return false;
}

bool FUserOnlineAccountZomboy::GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const
{
	const FString* FoundAttr = UserAttributes.Find(AttrName);
	if (FoundAttr != NULL)
	{
		OutAttrValue = *FoundAttr;
		return true;
	}
	return false;
}

bool FUserOnlineAccountZomboy::SetUserAttribute(const FString& AttrName, const FString& AttrValue)
{
	const FString* FoundAttr = UserAttributes.Find(AttrName);
	if (FoundAttr == NULL || *FoundAttr != AttrValue)
	{
		UserAttributes.Add(AttrName, AttrValue);
		return true;
	}
	return false;
}


bool FOnlineIdentityZomboy::Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials)
{
	UE_LOG(LogTemp, Log, TEXT("FOnlineIdentityZomboy::Login"));

	FString ErrorStr;
	TSharedPtr<FUserOnlineAccountZomboy> UserAccountPtr;

	// valid local player index
	if (LocalUserNum < 0 || LocalUserNum >= MAX_LOCAL_PLAYERS)
	{
		ErrorStr = FString::Printf(TEXT("Invalid LocalUserNum=%d"), LocalUserNum);
	}
	else if (AccountCredentials.Id.IsEmpty())
	{
		ErrorStr = TEXT("Invalid account id, string empty");
	}
	else
	{
		TSharedPtr<const FUniqueNetId>* UserId = UserIds.Find(LocalUserNum);
		if (UserId == NULL)
		{
#if WITH_STEAM

			if (SteamUser())
			{
				CSteamID SteamId = SteamUser()->GetSteamID();

				ensure(SteamId.IsValid());

				TSharedPtr<const FUniqueNetId> NewUserId = MakeShareable(new FUniqueNetIdZomboyPlayer(SteamId.ConvertToUint64()));
				UserIds.Add(LocalUserNum, NewUserId);

				UserAccountPtr = MakeShareable(new FUserOnlineAccountZomboy(NewUserId.ToSharedRef(), GetPlayerNickname(*NewUserId)));
				UserAccounts.Add(static_cast<FUniqueNetIdZomboyPlayer>(*NewUserId), UserAccountPtr.ToSharedRef());
			}
			else
			{
				ErrorStr = TEXT("Steam isn't initialized.");
			}

#elif WITH_OCULUS

			auto OculusId = ovr_GetLoggedInUserID();
			if (OculusId == 0)
			{
				ErrorStr = TEXT("Not currently logged into Oculus.  Make sure Oculus is running and you are entitled to the app.");
				FPlatformMisc::RequestExit(false);
			}
			else
			{
				uint64 oculus_userid_u64 = OculusId;

				// Immediately add the Oculus ID to our cache list
				TSharedPtr<const FUniqueNetId> NewUserId = MakeShareable(new FUniqueNetIdZomboyPlayer(oculus_userid_u64));

				UserIds.Add(LocalUserNum, NewUserId);

				FOnlineAsyncTaskOculusConfirmLogin* NewTask = new FOnlineAsyncTaskOculusConfirmLogin(ZomboySubsystem);
				ZomboySubsystem->QueueAsyncTask(NewTask);

				return true;
			}
#else
			uint64 RandomId = FMath::Rand();

			TSharedPtr<const FUniqueNetId> NewUserId = MakeShareable(new FUniqueNetIdZomboyPlayer(RandomId));
			UserIds.Add(LocalUserNum, NewUserId);

			UserAccountPtr = MakeShareable(new FUserOnlineAccountZomboy(NewUserId.ToSharedRef(), GetPlayerNickname(*NewUserId)));
			UserAccounts.Add(static_cast<FUniqueNetIdZomboyPlayer>(*NewUserId), UserAccountPtr.ToSharedRef());

#endif // WITH_STEAM
		}
		else
		{
			const FUniqueNetIdZomboyPlayer* ZomboyUserId = (FUniqueNetIdZomboyPlayer*)(UserId->Get());
			TSharedRef<FUserOnlineAccountZomboy>* TempPtr = UserAccounts.Find(*ZomboyUserId);
			check(TempPtr);
			UserAccountPtr = *TempPtr;
		}
	}

	if (!ErrorStr.IsEmpty())
	{
		UE_LOG_ONLINE(Warning, TEXT("Login request failed. %s"), *ErrorStr);
		TriggerOnLoginCompleteDelegates(LocalUserNum, false, FUniqueNetIdString(), ErrorStr);
		return false;
	}

	TriggerOnLoginCompleteDelegates(LocalUserNum, true, *UserAccountPtr->GetUserId(), ErrorStr);
	return true;
}

bool FOnlineIdentityZomboy::Logout(int32 LocalUserNum)
{
	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		// remove cached user id
		UserIds.Remove(LocalUserNum);
		// not async but should call completion delegate anyway
		TriggerOnLogoutCompleteDelegates(LocalUserNum, true);

		return true;
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("No logged in user found for LocalUserNum=%d."),
			LocalUserNum);
		TriggerOnLogoutCompleteDelegates(LocalUserNum, false);
	}
	return false;
}

bool FOnlineIdentityZomboy::AutoLogin(int32 LocalUserNum)
{
	UE_LOG(LogTemp, Log, TEXT("FOnlineIdentityZomboy::AutoLogin"));

	FString LoginStr;
	FString PasswordStr;
	FString TypeStr;

	FParse::Value(FCommandLine::Get(), TEXT("AUTH_LOGIN="), LoginStr);
	FParse::Value(FCommandLine::Get(), TEXT("AUTH_PASSWORD="), PasswordStr);
	FParse::Value(FCommandLine::Get(), TEXT("AUTH_TYPE="), TypeStr);
	
	if (!LoginStr.IsEmpty())
	{
		if (!PasswordStr.IsEmpty())
		{
			if (!TypeStr.IsEmpty())
			{
				return Login(0, FOnlineAccountCredentials(TypeStr, LoginStr, PasswordStr));
			}
			else
			{
				UE_LOG_ONLINE(Warning, TEXT("AutoLogin missing AUTH_TYPE=<type>."));
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("AutoLogin missing AUTH_PASSWORD=<password>."));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("AutoLogin missing AUTH_LOGIN=<login id>."));
	}
	return false;
}

TSharedPtr<FUserOnlineAccount> FOnlineIdentityZomboy::GetUserAccount(const FUniqueNetId& UserId) const
{
	TSharedPtr<FUserOnlineAccount> Result;

	FUniqueNetIdZomboyPlayer ZomboyUserId(UserId);

	const TSharedRef<FUserOnlineAccountZomboy>* FoundUserAccount = UserAccounts.Find(ZomboyUserId);
	if (FoundUserAccount != NULL)
	{
		Result = *FoundUserAccount;
	}

	return Result;
}

TArray<TSharedPtr<FUserOnlineAccount> > FOnlineIdentityZomboy::GetAllUserAccounts() const
{
	TArray<TSharedPtr<FUserOnlineAccount> > Result;

	for (TMap<FUniqueNetIdZomboyPlayer, TSharedRef<FUserOnlineAccountZomboy>>::TConstIterator It(UserAccounts); It; ++It)
	{
		Result.Add(It.Value());
	}

	return Result;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityZomboy::GetUniquePlayerId(int32 LocalUserNum) const
{
	const TSharedPtr<const FUniqueNetId>* FoundId = UserIds.Find(LocalUserNum);
	if (FoundId != NULL)
	{
		return *FoundId;
	}
	return NULL;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityZomboy::CreateUniquePlayerId(uint8* Bytes, int32 Size)
{
	if (Bytes && Size == sizeof(uint64))
	{
		uint64* RawUniqueId = (uint64*)Bytes;
		return MakeShareable(new FUniqueNetIdZomboyPlayer(*RawUniqueId));
	}

	return NULL;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityZomboy::CreateUniquePlayerId(const FString& Str)
{
	return MakeShareable(new FUniqueNetIdZomboyPlayer(Str));
}

ELoginStatus::Type FOnlineIdentityZomboy::GetLoginStatus(int32 LocalUserNum) const
{
	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		return GetLoginStatus(*UserId);
	}
	return ELoginStatus::NotLoggedIn;
}

ELoginStatus::Type FOnlineIdentityZomboy::GetLoginStatus(const FUniqueNetId& UserId) const 
{
	TSharedPtr<FUserOnlineAccount> UserAccount = GetUserAccount(UserId);
	if (UserAccount.IsValid() &&
		UserAccount->GetUserId()->IsValid())
	{
		return ELoginStatus::LoggedIn;
	}
	return ELoginStatus::NotLoggedIn;
}

FString FOnlineIdentityZomboy::GetPlayerNickname(int32 LocalUserNum) const
{
	TSharedPtr<const FUniqueNetId> UniqueId = GetUniquePlayerId(LocalUserNum);
	if (UniqueId.IsValid())
	{
		return GetPlayerNickname(*UniqueId);
	}

	return TEXT("ZomboyUser");
}

FString FOnlineIdentityZomboy::GetPlayerNickname(const FUniqueNetId& UserId) const
{
#if WITH_STEAM
	if (SteamFriends() != NULL)
	{
		const char* PersonaName = SteamFriends()->GetPersonaName();
		return FString(UTF8_TO_TCHAR(PersonaName));
	}
	else
	{
		return UserId.ToString();
	}
#elif WITH_OCULUS

	auto UserAccount = GetUserAccount(UserId);
	if (UserAccount.IsValid())
	{
		return UserAccount->GetDisplayName();
	}
	return UserId.ToString();

#else
	return UserId.ToString();
#endif
}

void FOnlineIdentityZomboy::LoadPlayerAvatarIcon(int32 LocalUserNum)
{
	TSharedPtr<const FUniqueNetId> PlayerNetId = GetUniquePlayerId(LocalUserNum);
	if (PlayerNetId.IsValid() && PlayerNetId->IsValid())
	{
		LoadPlayerAvatarIcon(*PlayerNetId.Get());
		return;
	}

	TriggerOnLoadPlayerIconCompleteDelegates(0, false, FZomboyOnlinePlayerIconInfo());
}

void FOnlineIdentityZomboy::LoadPlayerAvatarIcon(const FUniqueNetId& UserId)
{
#if WITH_STEAM
	 LoadPlayerAvatarIconSteam(UserId);
	 return;
#elif WITH_OCULUS
	 LoadPlayerAvatarIconOculus(UserId);
	 return;
#endif
	 TriggerOnLoadPlayerIconCompleteDelegates(0, false, FZomboyOnlinePlayerIconInfo());
}

#if WITH_STEAM

void FOnlineIdentityZomboy::LoadPlayerAvatarIconSteam(const FUniqueNetId& UserId)
{
	if (UserId.IsValid())
	{
		//uint64 PlayerId = FCString::Atoi64(*UserId.ToString());
		uint64 PlayerId = static_cast<FUniqueNetIdZomboyPlayer>(UserId).UniqueNetId64;

		CSteamID SteamId = PlayerId;

		uint32 Width = 0;
		uint32 Height = 0;

		int Picture = 0;

		if (SteamFriends() && SteamUtils() && SteamId.IsValid())
		{
			// get the Avatar ID using the player ID
			Picture = SteamFriends()->GetMediumFriendAvatar(SteamId);

			//if the Avatar ID is not valid retrun null
			if (Picture == -1)
				return;

			//get the image size from steam
			SteamUtils()->GetImageSize(Picture, &Width, &Height);

			// if the image size is valid (most of this is from the Advanced Seesion Plugin as well, mordentral, THANK YOU!
			if (Width > 0 && Height > 0)
			{
				//Creating the buffer "oAvatarRGBA" and then filling it with the RGBA Stream from the Steam Avatar
				uint8 *oAvatarRGBA = new uint8[Width * Height * 4];

				//Filling the buffer with the RGBA Stream from the Steam Avatar and creating a UTextur2D to parse the RGBA Steam in
				SteamUtils()->GetImageRGBA(Picture, (uint8*)oAvatarRGBA, 4 * Height * Width * sizeof(char));

				TArray<uint8> RawData;
				RawData.AddUninitialized(Width * Height * 4);
				FMemory::Memcpy(RawData.GetData(), oAvatarRGBA, Width * Height * 4);

				delete[] oAvatarRGBA;

				FZomboyOnlinePlayerIconInfo IconInfo(Width, Height, RawData);

				if (IconInfo.IsValid())
				{
					TriggerOnLoadPlayerIconCompleteDelegates(0, true, IconInfo);
					return;
				}
			}
		}
	}

	TriggerOnLoadPlayerIconCompleteDelegates(0, false, FZomboyOnlinePlayerIconInfo());
}
#elif WITH_OCULUS
void FOnlineIdentityZomboy::LoadPlayerAvatarIconOculus(const FUniqueNetId& UserId)
{
	if (UserId.IsValid())
	{
		uint64 PlayerId = FCString::Atoi64(*UserId.ToString());
		if (PlayerId > 0)
		{
			FOnlineAsyncTaskOculusLoadPlayerImage* NewTask = new FOnlineAsyncTaskOculusLoadPlayerImage(ZomboySubsystem);
			ZomboySubsystem->AddParallelAsyncTask(NewTask);
		}
	}
}
#endif

FString FOnlineIdentityZomboy::GetAuthToken(int32 LocalUserNum) const
{
	TSharedPtr<const FUniqueNetId> UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		TSharedPtr<FUserOnlineAccount> UserAccount = GetUserAccount(*UserId);
		if (UserAccount.IsValid())
		{
			return UserAccount->GetAccessToken();
		}
	}
	return FString();
}

void FOnlineIdentityZomboy::RevokeAuthToken(const FUniqueNetId& UserId, const FOnRevokeAuthTokenCompleteDelegate& Delegate)
{
	UE_LOG(LogOnline, Display, TEXT("FOnlineIdentityZomboy::RevokeAuthToken not implemented"));
	TSharedRef<const FUniqueNetId> UserIdRef(UserId.AsShared());
	ZomboySubsystem->ExecuteNextTick([UserIdRef, Delegate]()
	{
		Delegate.ExecuteIfBound(*UserIdRef, FOnlineError(FString(TEXT("RevokeAuthToken not implemented"))));
	});
}

FOnlineIdentityZomboy::FOnlineIdentityZomboy(FOnlineSubsystemZomboy* InSubsystem)
	: ZomboySubsystem(InSubsystem)
{
	// autologin the 0-th player
	Login(0, FOnlineAccountCredentials(TEXT("DummyType"), TEXT("DummyUser"), TEXT("DummyId")) );
}

FOnlineIdentityZomboy::~FOnlineIdentityZomboy()
{
}

void FOnlineIdentityZomboy::GetUserPrivilege(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, const FOnGetUserPrivilegeCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(UserId, Privilege, (uint32)EPrivilegeResults::NoFailures);
}

FPlatformUserId FOnlineIdentityZomboy::GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& UniqueNetId) const
{
	for (int i = 0; i < MAX_LOCAL_PLAYERS; ++i)
	{
		auto CurrentUniqueId = GetUniquePlayerId(i);
		if (CurrentUniqueId.IsValid() && (*CurrentUniqueId == UniqueNetId))
		{
			return i;
		}
	}

	return PLATFORMUSERID_NONE;
}

FString FOnlineIdentityZomboy::GetAuthType() const
{
	return TEXT("");
}

void FOnlineIdentityZomboy::AddUniquePlayerId(int32 LocalUserNum, TSharedPtr<const FUniqueNetId> UserUniqueNetId)
{
	if (UserUniqueNetId.IsValid())
	{
		UserIds.Add(LocalUserNum, UserUniqueNetId);
	}
}

void FOnlineIdentityZomboy::AddUserAccount(FUniqueNetIdZomboyPlayer UserUniqueNetId, TSharedRef<FUserOnlineAccountZomboy> UserAccount)
{
	UserAccounts.Add(UserUniqueNetId, UserAccount);
}

