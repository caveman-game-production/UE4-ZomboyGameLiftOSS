// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemImpl.h"
#include "OnlineSubsystemZomboyPackage.h"
#include "HAL/ThreadSafeCounter.h"
#include "OnlineSubsystemZomboyTypes.h"

class FOnlineIdentityZomboy;
class FOnlineSessionZomboy;
class FOnlineVoiceImpl;

/** Forward declarations of all interface classes */
typedef TSharedPtr<class FOnlineSessionZomboy, ESPMode::ThreadSafe> FOnlineSessionZomboyPtr;
typedef TSharedPtr<class FOnlineIdentityZomboy, ESPMode::ThreadSafe> FOnlineIdentityZomboyPtr;


DECLARE_LOG_CATEGORY_EXTERN(GameLiftLog, Log, All);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGameLiftRegionChange, EGameLiftRegion /*GameLift Client Region*/);


/**
 *	OnlineSubsystemNull - Implementation of the online subsystem for Null services
 */
class ONLINESUBSYSTEMZOMBOY_API FOnlineSubsystemZomboy : 
	public FOnlineSubsystemImpl
{


#if WITH_GAMELIFTCLIENTSDK

protected:
	bool InitGameLiftModules();

	TSharedPtr<class FGameLiftClientObject, ESPMode::ThreadSafe> GameLiftClient;

public:
	inline TSharedPtr<class FGameLiftClientObject, ESPMode::ThreadSafe> GetGameLiftClient() { return GameLiftClient; }
	FString GetGameLiftMatchmakingConfigName();
	FString GetGameLiftQueueName();

	void SetGameLiftAliasID(const FString& InGameLiftAliasID);

	void SetGameLiftClientRegion(EGameLiftRegion Region);
	void UpdateSession(const FString SessionId, const FString& SessionInfo);

#endif

#if WITH_STEAM
protected:

	/** Has the STEAM client APIs been initialized */
	bool bSteamworksClientInitialized;

	bool InitSteamworksClient(bool bRelaunchInSteam, int32 SteamAppId);
	void ShutdownSteamworks();
#endif

#if WITH_OCULUS
protected:

	bool InitOculusWithWindowsPlatform() const;
	void ShutdownOculusWithWindowsPlatform() const;

#endif

public:

	virtual ~FOnlineSubsystemZomboy()
	{
	}

	// IOnlineSubsystem

	virtual IOnlineSessionPtr GetSessionInterface() const override;
	virtual IOnlineFriendsPtr GetFriendsInterface() const override;
	virtual IOnlinePartyPtr GetPartyInterface() const override;
	virtual IOnlineGroupsPtr GetGroupsInterface() const override;
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const override;
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const override;
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const override;
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const override;
	virtual IOnlineVoicePtr GetVoiceInterface() const override;
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const override;
	virtual IOnlineTimePtr GetTimeInterface() const override;
	virtual IOnlineIdentityPtr GetIdentityInterface() const override;
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const override;
	virtual IOnlineStorePtr GetStoreInterface() const override;
	virtual IOnlineStoreV2Ptr GetStoreV2Interface() const override { return nullptr; }
	virtual IOnlinePurchasePtr GetPurchaseInterface() const override { return nullptr; }
	virtual IOnlineEventsPtr GetEventsInterface() const override;
	virtual IOnlineAchievementsPtr GetAchievementsInterface() const override;
	virtual IOnlineSharingPtr GetSharingInterface() const override;
	virtual IOnlineUserPtr GetUserInterface() const override;
	virtual IOnlineMessagePtr GetMessageInterface() const override;
	virtual IOnlinePresencePtr GetPresenceInterface() const override;
	virtual IOnlineChatPtr GetChatInterface() const override;
	virtual IOnlineTurnBasedPtr GetTurnBasedInterface() const override;

	void QueueAsyncTask(class FOnlineAsyncTask* AsyncTask);
	void AddParallelAsyncTask(class FOnlineAsyncTask* AsyncTask);


	virtual bool Init() override;
	virtual bool Shutdown() override;
	virtual FString GetAppId() const override;
	virtual FText GetOnlineServiceName() const override;

	// FTickerObjectBase
	virtual bool Tick(float DeltaTime) override;

	// FOnlineSubsystemZomboy

	inline bool IsOnlinePlatformAvailable()
	{
		return bOnlinePlatformInitialzed;
	}

	inline class FOnlineAsyncTaskManagerZomboy* GetAsyncTaskManager() 
	{
#if !WITH_GAMELIFT_SERVER
		return OnlineAsyncTaskThreadRunnable; 
#endif
		return nullptr;
	}

	inline class FOnlineAsyncTaskManagerZomboyServer* GetServerAsyncTaskManager()
	{
#if WITH_GAMELIFT_SERVER
		return OnlineServerAsyncTaskThreadRunnable;
#endif
		return nullptr;
	}


PACKAGE_SCOPE:

	/** Only the factory makes instances */
	FOnlineSubsystemZomboy(FName InInstanceName) :
		FOnlineSubsystemImpl(ZOMBOY_SUBSYSTEM, InInstanceName),
		SessionInterfaceZomboy(nullptr),
		VoiceInterfaceZomboy(nullptr),
		IdentityInterfaceZomboy(nullptr),
		bOnlinePlatformInitialzed(false),
		bVoiceInterfaceInitialized(false),
#if WITH_GAMELIFT_SERVER
		OnlineServerAsyncTaskThreadRunnable(nullptr),
#else
		OnlineAsyncTaskThreadRunnable(nullptr),
#endif	
		OnlineAsyncTaskThread(nullptr)

	{}

	FOnlineSubsystemZomboy() :
		SessionInterfaceZomboy(nullptr),
		VoiceInterfaceZomboy(nullptr),
		IdentityInterfaceZomboy(nullptr),
		bOnlinePlatformInitialzed(false),
		bVoiceInterfaceInitialized(false),
#if WITH_GAMELIFT_SERVER
		OnlineServerAsyncTaskThreadRunnable(nullptr),
#else
		OnlineAsyncTaskThreadRunnable(nullptr),
#endif
		OnlineAsyncTaskThread(nullptr)
	{}

public:
	FOnGameLiftRegionChange OnGameLiftRegionChange;

private:

	bool bOnlinePlatformInitialzed;

	FString GameLiftAliasID;
	FString GameLiftMatchmakingConfigName;
	FString GameLiftSessionQueueName;

	/** Interface to the session services */
	FOnlineSessionZomboyPtr SessionInterfaceZomboy;

	/** Interface for voice communication */
	mutable IOnlineVoicePtr VoiceInterfaceZomboy;
	/** Interface for voice communication */
	mutable bool bVoiceInterfaceInitialized;

	/** Interface to the identity registration/auth services */
	FOnlineIdentityZomboyPtr IdentityInterfaceZomboy;

	/** Online async task runnable */
#if WITH_GAMELIFT_SERVER
	class FOnlineAsyncTaskManagerZomboyServer* OnlineServerAsyncTaskThreadRunnable;
#else
	class FOnlineAsyncTaskManagerZomboy* OnlineAsyncTaskThreadRunnable;
#endif

	/** Online async task thread */
	class FRunnableThread* OnlineAsyncTaskThread;

	// task counter, used to generate unique thread names for each task
	static FThreadSafeCounter TaskCounter;

};

typedef TSharedPtr<FOnlineSubsystemZomboy, ESPMode::ThreadSafe> FOnlineSubsystemZomboyPtr;

