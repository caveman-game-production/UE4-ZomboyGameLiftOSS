// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemZomboy.h"
#include "OnlineSubsystemZomboyPrivate.h"

#include "HAL/RunnableThread.h"
#include "OnlineAsyncTaskManagerZomboy.h"

#include "OnlineIdentityZomboy.h"
#include "OnlineSessionInterfaceZomboy.h"
#include "VoiceInterfaceImpl.h"

#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Misc/ConfigCacheIni.h"
#include "Serialization/BufferArchive.h"


DEFINE_LOG_CATEGORY(GameLiftLog);

FThreadSafeCounter FOnlineSubsystemZomboy::TaskCounter;

#if WITH_GAMELIFTCLIENTSDK

bool FOnlineSubsystemZomboy::InitGameLiftModules()
{
	const FName GameLiftCoreModuleName = TEXT("AWSCore");
	const FName GameLiftClientModuleName = TEXT("GameLiftClientSDK");

	FModuleManager& ModuleManager = FModuleManager::Get();

	if (!ModuleManager.IsModuleLoaded(GameLiftCoreModuleName))
	{
		// Attempt to load the module
		if (ModuleManager.LoadModule(GameLiftCoreModuleName) == NULL)
		{
			return false;
		}
	}

	if (!ModuleManager.IsModuleLoaded(GameLiftClientModuleName))
	{
		// Attempt to load the module
		if (ModuleManager.LoadModule(GameLiftClientModuleName) == NULL)
		{
			return false;
		}
	}

	return true;
}

FString FOnlineSubsystemZomboy::GetGameLiftMatchmakingConfigName()
{
	return GameLiftMatchmakingConfigName;
}

FString FOnlineSubsystemZomboy::GetGameLiftQueueName()
{
	return GameLiftSessionQueueName;
}

void FOnlineSubsystemZomboy::SetGameLiftAliasID(const FString& InGameLiftAliasID)
{
	GameLiftAliasID = InGameLiftAliasID;
}

void FOnlineSubsystemZomboy::SetGameLiftClientRegion(EGameLiftRegion Region)
{
	if (Region != GameLiftClient->GetCurrentRegion())
	{
		if (GameLiftClient->SetCurrentRegion(Region))
		{
			OnGameLiftRegionChange.Broadcast(Region);
		}
	}
}

void FOnlineSubsystemZomboy::UpdateSession(const FString SessionId, const FString& SessionInfo)
{
	GameLiftClient->UpdateGameSession(SessionId, SessionInfo);
}

void ConfigureGameLiftInit(FString& Key, FString& Secret, FString& MatchmakingConfigName, FString& SessionQueueName)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_SHIPPING_WITH_EDITOR

	if (!GConfig->GetString(TEXT("OnlineSubsystemZomboy"), TEXT("GameLiftDevAccessKey"), Key, GEngineIni))
	{
		UE_LOG_ONLINE(Warning, TEXT("Missing GameLiftAccessKey in OnlineSubsystemZomboy of DefaultEngine.ini"));
	}

	if (!GConfig->GetString(TEXT("OnlineSubsystemZomboy"), TEXT("GameLiftDevSecret"), Secret, GEngineIni))
	{
		UE_LOG_ONLINE(Warning, TEXT("Missing GameLiftSecret in OnlineSubsystemZomboy of DefaultEngine.ini"));
	}

	if (!GConfig->GetString(TEXT("OnlineSubsystemZomboy"), TEXT("GameLiftMatchmakingConfigName"), MatchmakingConfigName, GEngineIni))
	{
		UE_LOG_ONLINE(Warning, TEXT("Missing MatchmakingConfigName in OnlineSubsystemZomboy of DefaultEngine.ini"));
	}

	if (!GConfig->GetString(TEXT("OnlineSubsystemZomboy"), TEXT("GameLiftSessionQueueName"), SessionQueueName, GEngineIni))
	{
		UE_LOG_ONLINE(Warning, TEXT("Missing MatchmakingConfigName in OnlineSubsystemZomboy of DefaultEngine.ini"));
	}

#else
	//Enter gamelift accesskey and secret here when you are ready to package.
	Key = TEXT("Your Key");
	Secret = TEXT("Your Secret");
	MatchmakingConfigName = TEXT("Your Matchmaking Config Name");
	SessionQueueName = TEXT("Your Session Queue Name");
#endif
}


#endif

#if WITH_STEAM

extern "C"
{
	static void __cdecl SteamworksWarningMessageHook(int Severity, const char *Message);
	static void __cdecl SteamworksWarningMessageHookNoOp(int Severity, const char *Message);
}

/**
* Callback function into Steam error messaging system
* @param Severity - error level
* @param Message - message from Steam
*/
static void __cdecl SteamworksWarningMessageHook(int Severity, const char *Message)
{
	const TCHAR *MessageType;
	switch (Severity)
	{
	case 0: MessageType = TEXT("message"); break;
	case 1: MessageType = TEXT("warning"); break;
	default: MessageType = TEXT("notification"); break;  // Unknown severity; new SDK?
	}
	UE_LOG_ONLINE(Warning, TEXT("Steamworks SDK %s: %s"), MessageType, UTF8_TO_TCHAR(Message));
}

/**
* Callback function into Steam error messaging system that outputs nothing
* @param Severity - error level
* @param Message - message from Steam
*/
static void __cdecl SteamworksWarningMessageHookNoOp(int Severity, const char *Message)
{
	// no-op.
}

class FScopeSandboxContext
{
private:
	/** Previous state of sandbox enable */
	bool bSandboxWasEnabled;

	FScopeSandboxContext() {}

public:
	FScopeSandboxContext(bool bSandboxEnabled)
	{
		bSandboxWasEnabled = IFileManager::Get().IsSandboxEnabled();
		IFileManager::Get().SetSandboxEnabled(bSandboxEnabled);
	}

	~FScopeSandboxContext()
	{
		IFileManager::Get().SetSandboxEnabled(bSandboxWasEnabled);
	}
};

inline FString GetSteamAppIdFilename()
{
	return FString::Printf(TEXT("%s%s"), FPlatformProcess::BaseDir(), STEAMAPPIDFILENAME);
}

#if !UE_BUILD_SHIPPING && !UE_BUILD_SHIPPING_WITH_EDITOR
/**
* Write out the steam app id to the steam_appid.txt file before initializing the API
* @param SteamAppId id assigned to the application by Steam
*/
static void WriteSteamAppIdToDisk(int32 SteamAppId)
{
	if (SteamAppId > 0)
	{
		// Turn off sandbox temporarily to make sure file is where it's always expected
		FScopeSandboxContext ScopedSandbox(false);

		// Access the physical file writer directly so that we still write next to the executable in CotF builds.
		FString SteamAppIdFilename = GetSteamAppIdFilename();
		IFileHandle* Handle = IPlatformFile::GetPlatformPhysical().OpenWrite(*SteamAppIdFilename, false, false);
		if (!Handle)
		{
			UE_LOG_ONLINE(Fatal, TEXT("Failed to create file: %s"), *SteamAppIdFilename);
		}
		else
		{
			FString AppId = FString::Printf(TEXT("%d"), SteamAppId);

			FBufferArchive Archive;
			Archive.Serialize((void*)TCHAR_TO_ANSI(*AppId), AppId.Len());

			Handle->Write(Archive.GetData(), Archive.Num());
			delete Handle;
			Handle = nullptr;
		}
	}
}

/**
* Deletes the app id file from disk
*/
static void DeleteSteamAppIdFromDisk()
{
	const FString SteamAppIdFilename = GetSteamAppIdFilename();
	// Turn off sandbox temporarily to make sure file is where it's always expected
	FScopeSandboxContext ScopedSandbox(false);
	if (FPaths::FileExists(*SteamAppIdFilename))
	{
		bool bSuccessfullyDeleted = IFileManager::Get().Delete(*SteamAppIdFilename);
	}
}

#endif // !UE_BUILD_SHIPPING && !UE_BUILD_SHIPPING_WITH_EDITOR

/**
* Configure various dev options before initializing Steam
*
* @param RequireRelaunch enforce the Steam client running precondition
* @param RelaunchAppId appid to launch when the Steam client is loaded
*/
void ConfigureSteamInitDevOptions(bool& RequireRelaunch, int32& RelaunchAppId)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_SHIPPING_WITH_EDITOR
	// Write out the steam_appid.txt file before launching
	if (!GConfig->GetInt(TEXT("OnlineSubsystemZomboy"), TEXT("SteamDevAppId"), RelaunchAppId, GEngineIni))
	{
		UE_LOG_ONLINE(Warning, TEXT("Missing SteamDevAppId key in OnlineSubsystemZomboy of DefaultEngine.ini"));
	}
	else
	{
		WriteSteamAppIdToDisk(RelaunchAppId);
	}

	// Should the game force a relaunch in Steam if the client isn't already loaded
	if (!GConfig->GetBool(TEXT("OnlineSubsystemSteam"), TEXT("bRelaunchInSteam"), RequireRelaunch, GEngineIni))
	{
		UE_LOG_ONLINE(Warning, TEXT("Missing bRelaunchInSteam key in OnlineSubsystemSteam of DefaultEngine.ini"));
	}

#else
	// Always check against the Steam client when shipping
	RequireRelaunch = true;
	// Enter shipping app id here
	RelaunchAppId = 0;
#endif
}

bool FOnlineSubsystemZomboy::InitSteamworksClient(bool bRelaunchInSteam, int32 SteamAppId)
{
	bSteamworksClientInitialized = false;

	// If the game was not launched from within Steam, but is supposed to, trigger a Steam launch and exit
	if (bRelaunchInSteam && SteamAppId != 0 && SteamAPI_RestartAppIfNecessary(SteamAppId))
	{
		if (FPlatformProperties::IsGameOnly() || FPlatformProperties::IsServerOnly())
		{
			UE_LOG_ONLINE(Log, TEXT("Game restarting within Steam client, exiting"));
			FPlatformMisc::RequestExit(false);
		}

		return bSteamworksClientInitialized;
	}
	// Otherwise initialize as normal
	else
	{
		// Steamworks needs to initialize as close to start as possible, so it can hook its overlay into Direct3D, etc.
		bSteamworksClientInitialized = (SteamAPI_Init() ? true : false);

		// Test all the Steam interfaces
#define GET_STEAMWORKS_INTERFACE(Interface) \
		if (Interface() == nullptr) \
		{ \
			UE_LOG_ONLINE(Warning, TEXT("Steamworks: %s() failed!"), TEXT(#Interface)); \
			bSteamworksClientInitialized = false; \
		} \

		// GSteamUtils
		GET_STEAMWORKS_INTERFACE(SteamUtils);
		// GSteamUser
		GET_STEAMWORKS_INTERFACE(SteamUser);
		// GSteamFriends
		GET_STEAMWORKS_INTERFACE(SteamFriends);
		// GSteamRemoteStorage
		GET_STEAMWORKS_INTERFACE(SteamRemoteStorage);
		// GSteamUserStats
		GET_STEAMWORKS_INTERFACE(SteamUserStats);
		// GSteamMatchmakingServers
		GET_STEAMWORKS_INTERFACE(SteamMatchmakingServers);
		// GSteamApps
		GET_STEAMWORKS_INTERFACE(SteamApps);
		// GSteamNetworking
		GET_STEAMWORKS_INTERFACE(SteamNetworking);
		// GSteamMatchmaking
		GET_STEAMWORKS_INTERFACE(SteamMatchmaking);

#undef GET_STEAMWORKS_INTERFACE
	}

	if (bSteamworksClientInitialized)
	{
		bool bIsSubscribed = true;
		if (FPlatformProperties::IsGameOnly() || FPlatformProperties::IsServerOnly())
		{
			bIsSubscribed = SteamApps()->BIsSubscribed();
		}

		// Make sure the Steam user has valid access to the game
		if (bIsSubscribed)
		{
			UE_LOG_ONLINE(Log, TEXT("Steam User is subscribed %i"), bSteamworksClientInitialized);
			if (SteamUtils())
			{
				SteamUtils()->SetWarningMessageHook(SteamworksWarningMessageHook);
			}
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("Steam User is NOT subscribed, exiting."));
			bSteamworksClientInitialized = false;
			FPlatformMisc::RequestExit(false);
		}
	}

	UE_LOG_ONLINE(Log, TEXT("[AppId: %d] Client API initialized %i"), SteamUtils()->GetAppID(), bSteamworksClientInitialized);
	return bSteamworksClientInitialized;
}

void FOnlineSubsystemZomboy::ShutdownSteamworks()
{
	if (bSteamworksClientInitialized)
	{
		SteamAPI_Shutdown();
		bSteamworksClientInitialized = false;
	}
}

#endif //WITH_STEAM

#if WITH_OCULUS

#if !defined(OVRPL_DISABLED) && WITH_EDITOR
OVRPL_PUBLIC_FUNCTION(void) ovr_ResetInitAndContext();
#endif

bool FOnlineSubsystemZomboy::InitOculusWithWindowsPlatform() const
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSubsystemOculus::InitWithWindowsPlatform()"));
	auto OculusAppId = GetAppId();
	if (OculusAppId.IsEmpty())
	{
		UE_LOG_ONLINE(Warning, TEXT("Missing OculusAppId key in OnlineSubsystemOculus of DefaultEngine.ini"));
		return false;
	}

	auto InitResult = ovr_PlatformInitializeWindows(TCHAR_TO_ANSI(*OculusAppId));
	if (InitResult != ovrPlatformInitialize_Success)
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to initialize the Oculus Platform SDK! Failure code: %d"), static_cast<int>(InitResult));
		return false;
	}
	return true;
}

void FOnlineSubsystemZomboy::ShutdownOculusWithWindowsPlatform() const
{
#if !defined(OVRPL_DISABLED) && WITH_EDITOR
	// If we are playing in the editor,
	// Destroy the context and reset the init status
	ovr_ResetInitAndContext();
#endif
}

#endif

IOnlineSessionPtr FOnlineSubsystemZomboy::GetSessionInterface() const
{
	return SessionInterfaceZomboy;
}

IOnlineFriendsPtr FOnlineSubsystemZomboy::GetFriendsInterface() const
{
	return nullptr;
}

IOnlinePartyPtr FOnlineSubsystemZomboy::GetPartyInterface() const
{
	return nullptr;
}

IOnlineGroupsPtr FOnlineSubsystemZomboy::GetGroupsInterface() const
{
	return nullptr;
}

IOnlineSharedCloudPtr FOnlineSubsystemZomboy::GetSharedCloudInterface() const
{
	return nullptr;
}

IOnlineUserCloudPtr FOnlineSubsystemZomboy::GetUserCloudInterface() const
{
	return nullptr;
}

IOnlineEntitlementsPtr FOnlineSubsystemZomboy::GetEntitlementsInterface() const
{
	return nullptr;
};

IOnlineLeaderboardsPtr FOnlineSubsystemZomboy::GetLeaderboardsInterface() const
{
	return nullptr;
}

IOnlineVoicePtr FOnlineSubsystemZomboy::GetVoiceInterface() const
{
	if (VoiceInterfaceZomboy.IsValid() && !bVoiceInterfaceInitialized)
	{
		if (!VoiceInterfaceZomboy->Init())
		{
			VoiceInterfaceZomboy = nullptr;
		}

		bVoiceInterfaceInitialized = true;
	}

	return VoiceInterfaceZomboy;
}

IOnlineExternalUIPtr FOnlineSubsystemZomboy::GetExternalUIInterface() const
{
	return nullptr;
}

IOnlineTimePtr FOnlineSubsystemZomboy::GetTimeInterface() const
{
	return nullptr;
}

IOnlineIdentityPtr FOnlineSubsystemZomboy::GetIdentityInterface() const
{
	return IdentityInterfaceZomboy;
}

IOnlineTitleFilePtr FOnlineSubsystemZomboy::GetTitleFileInterface() const
{
	return nullptr;
}

IOnlineStorePtr FOnlineSubsystemZomboy::GetStoreInterface() const
{
	return nullptr;
}

IOnlineEventsPtr FOnlineSubsystemZomboy::GetEventsInterface() const
{
	return nullptr;
}

IOnlineAchievementsPtr FOnlineSubsystemZomboy::GetAchievementsInterface() const
{
	return nullptr;
}

IOnlineSharingPtr FOnlineSubsystemZomboy::GetSharingInterface() const
{
	return nullptr;
}

IOnlineUserPtr FOnlineSubsystemZomboy::GetUserInterface() const
{
	return nullptr;
}

IOnlineMessagePtr FOnlineSubsystemZomboy::GetMessageInterface() const
{
	return nullptr;
}

IOnlinePresencePtr FOnlineSubsystemZomboy::GetPresenceInterface() const
{
	return nullptr;
}

IOnlineChatPtr FOnlineSubsystemZomboy::GetChatInterface() const
{
	return nullptr;
}

IOnlineTurnBasedPtr FOnlineSubsystemZomboy::GetTurnBasedInterface() const
{
	return nullptr;
}

void FOnlineSubsystemZomboy::QueueAsyncTask(FOnlineAsyncTask* AsyncTask)
{
#if !WITH_GAMELIFT_SERVER
	check(OnlineAsyncTaskThreadRunnable);
	OnlineAsyncTaskThreadRunnable->AddToInQueue(AsyncTask);
#endif 
}

void FOnlineSubsystemZomboy::AddParallelAsyncTask(FOnlineAsyncTask* AsyncTask)
{
#if !WITH_GAMELIFT_SERVER
	check(OnlineAsyncTaskThreadRunnable);
	OnlineAsyncTaskThreadRunnable->AddToParallelTasks(AsyncTask);
#endif
}

bool FOnlineSubsystemZomboy::Init()
{
	//Hack: load Media mudle here to prevent trigger break point.
	FModuleManager& ModuleManager = FModuleManager::Get();
	if (!ModuleManager.IsModuleLoaded(TEXT("Media")))
	{
		ModuleManager.LoadModule(TEXT("Media"));
	}

	bool bClientInitSuccess = true;

#if WITH_GAMELIFTCLIENTSDK
	bClientInitSuccess = InitGameLiftModules();
#endif

#if WITH_STEAM

	bool bRelaunchInSteam = false;
	int RelaunchAppId = 0;
	ConfigureSteamInitDevOptions(bRelaunchInSteam, RelaunchAppId);
	bClientInitSuccess = bClientInitSuccess ? InitSteamworksClient(bRelaunchInSteam, RelaunchAppId) : false;

#elif WITH_OCULUS

	bClientInitSuccess = bClientInitSuccess ? InitOculusWithWindowsPlatform() : false;

#endif

	if (bClientInitSuccess)
	{
#if WITH_GAMELIFTCLIENTSDK
		SetGameLiftAliasID(TEXT("alias-563bd985-1e37-4273-9a05-90ba22127c12"));

		FString AccessKey, Secret;
		ConfigureGameLiftInit(AccessKey, Secret, GameLiftMatchmakingConfigName, GameLiftSessionQueueName);
		//Create Gamelift Client Object
		GameLiftClient = FGameLiftClientObject::CreateGameLiftObject(AccessKey, Secret, EGameLiftRegion::EGameLiftRegion_NA, GAMELIFT_LOCAL, 8080);
#endif

#if WITH_GAMELIFT_SERVER
		// Create the online async task thread
		OnlineServerAsyncTaskThreadRunnable = new FOnlineAsyncTaskManagerZomboyServer(this);
		check(OnlineServerAsyncTaskThreadRunnable);
		OnlineAsyncTaskThread = FRunnableThread::Create(OnlineServerAsyncTaskThreadRunnable, *FString::Printf(TEXT("OnlineAsyncTaskThreadZomboy %s(%d)"), *InstanceName.ToString(), TaskCounter.Increment()), 1024 * 1024 * 2, TPri_Normal);
		check(OnlineAsyncTaskThread);
		UE_LOG_ONLINE(Verbose, TEXT("Created thread (ID:%d)."), OnlineAsyncTaskThread->GetThreadID());

#else

		// Create the online async task thread
		OnlineAsyncTaskThreadRunnable = new FOnlineAsyncTaskManagerZomboy(this);
		check(OnlineAsyncTaskThreadRunnable);
		OnlineAsyncTaskThread = FRunnableThread::Create(OnlineAsyncTaskThreadRunnable, *FString::Printf(TEXT("OnlineAsyncTaskThreadZomboy %s(%d)"), *InstanceName.ToString(), TaskCounter.Increment()), 128 * 1024, TPri_Normal);
		check(OnlineAsyncTaskThread);
		UE_LOG_ONLINE(Verbose, TEXT("Created thread (ID:%d)."), OnlineAsyncTaskThread->GetThreadID());

#endif

		SessionInterfaceZomboy = MakeShareable(new FOnlineSessionZomboy(this));
		IdentityInterfaceZomboy = MakeShareable(new FOnlineIdentityZomboy(this));
		VoiceInterfaceZomboy = MakeShareable(new FOnlineVoiceImpl(this));

		bOnlinePlatformInitialzed = true;

		return true;
	}
	else
	{
		Shutdown();
	}

	return false;
}

bool FOnlineSubsystemZomboy::Shutdown()
{
	/*bool Result= FOnlineSubsystemNull::Shutdown();

	if (IdentityInterfaceZomboy.IsValid())
	{
		ensure(IdentityInterfaceZomboy.IsUnique());
		IdentityInterfaceZomboy = nullptr;
	}*/

	UE_LOG_ONLINE(Display, TEXT("FOnlineSubsystemZomboy::Shutdown()"));

	FOnlineSubsystemImpl::Shutdown();

	if (OnlineAsyncTaskThread)
	{
		// Destroy the online async task thread
		delete OnlineAsyncTaskThread;
		OnlineAsyncTaskThread = nullptr;
	}

#if WITH_GAMELIFT_SERVER
	if (OnlineServerAsyncTaskThreadRunnable)
	{
		delete OnlineServerAsyncTaskThreadRunnable;
		OnlineServerAsyncTaskThreadRunnable = nullptr;
	}
#else
	if (OnlineAsyncTaskThreadRunnable)
	{
		delete OnlineAsyncTaskThreadRunnable;
		OnlineAsyncTaskThreadRunnable = nullptr;
	}
#endif

	if (VoiceInterfaceZomboy.IsValid() && bVoiceInterfaceInitialized)
	{
		VoiceInterfaceZomboy->Shutdown();
	}

#define DESTRUCT_INTERFACE(Interface) \
 	if (Interface.IsValid()) \
 	{ \
 		ensure(Interface.IsUnique()); \
 		Interface = nullptr; \
 	}

	// Destruct the interfaces
	DESTRUCT_INTERFACE(VoiceInterfaceZomboy);
	DESTRUCT_INTERFACE(IdentityInterfaceZomboy);
	DESTRUCT_INTERFACE(SessionInterfaceZomboy);

#undef DESTRUCT_INTERFACE

#if WITH_STEAM

	ShutdownSteamworks();

#if !UE_BUILD_SHIPPING && !UE_BUILD_SHIPPING_WITH_EDITOR
	DeleteSteamAppIdFromDisk();
#endif // !UE_BUILD_SHIPPING && !UE_BUILD_SHIPPING_WITH_EDITOR

#elif WITH_OCULUS

	ShutdownOculusWithWindowsPlatform();

#endif //WITH_STEAM

	return true;
}

FString FOnlineSubsystemZomboy::GetAppId() const
{
#if WITH_STEAM
#if !UE_BUILD_SHIPPING && !UE_BUILD_SHIPPING_WITH_EDITOR
	int32 SteamDevAppId = 0;
	if (!GConfig->GetInt(TEXT("OnlineSubsystemZomboy"), TEXT("SteamDevAppId"), SteamDevAppId, GEngineIni))
	{
		UE_LOG_ONLINE(Warning, TEXT("Missing SteamDevAppId key in OnlineSubsystemZomboy of DefaultEngine.ini"));
		return FString(TEXT(""));
	}
	else
	{
		return FString::FromInt(SteamDevAppId);
	}

#else 
	return FString::FromInt(963930);
#endif //!UE_BUILD_SHIPPING && !UE_BUILD_SHIPPING_WITH_EDITOR

#elif WITH_OCULUS
#if PLATFORM_WINDOWS
	auto AppId = GConfig->GetStr(TEXT("OnlineSubsystemZomboy"), TEXT("RiftAppId"), GEngineIni);
#elif PLATFORM_ANDROID
	auto AppId = GConfig->GetStr(TEXT("OnlineSubsystemZomboy"), TEXT("GearVRAppId"), GEngineIni);
#endif
	if (!AppId.IsEmpty()) {
		return AppId;
	}
#if PLATFORM_WINDOWS
	UE_LOG_ONLINE(Warning, TEXT("Could not find 'RiftAppId' key in engine config.  Trying 'OculusAppId'.  Move your oculus app id to 'RiftAppId' to use in your rift app and make this warning go away."));
#elif PLATFORM_ANDROID
	UE_LOG_ONLINE(Warning, TEXT("Could not find 'GearVRAppId' key in engine config.  Trying 'OculusAppId'.  Move your oculus app id to 'GearVRAppId' to use in your gearvr app make this warning go away."));
#endif
	return GConfig->GetStr(TEXT("OnlineSubsystemZomboy"), TEXT("OculusAppId"), GEngineIni);
#else
	//TODO: Need to implement for both Steam and Oculus
	return TEXT("6666");
#endif

}

FText FOnlineSubsystemZomboy::GetOnlineServiceName() const
{
	return NSLOCTEXT("OnlineSubsystemZomboy", "OnlineServiceName", "Zomboy");
}


bool FOnlineSubsystemZomboy::Tick(float DeltaTime)
{
	if (!FOnlineSubsystemImpl::Tick(DeltaTime))
	{
		return false;
	}

#if WITH_GAMELIFT_SERVER
	if (OnlineServerAsyncTaskThreadRunnable)
	{
		OnlineServerAsyncTaskThreadRunnable->GameServerTick();
	}
#else
	if (OnlineAsyncTaskThreadRunnable)
	{
		OnlineAsyncTaskThreadRunnable->GameTick();
	}
#endif

	if (SessionInterfaceZomboy.IsValid())
	{
		SessionInterfaceZomboy->Tick(DeltaTime);
	}

	if (VoiceInterfaceZomboy.IsValid() && bVoiceInterfaceInitialized)
	{
		VoiceInterfaceZomboy->Tick(DeltaTime);
	}

	return true;
}


