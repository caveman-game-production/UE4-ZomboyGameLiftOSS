// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineAsyncTaskManager.h"
#include "OnlineSubsystemZomboyPrivate.h"

#if WITH_GAMELIFT_SERVER
#include "GameLiftServerSDK.h"
#endif

#if WITH_GAMELIFTCLIENTSDK
class FOnlineAsyncTaskGameLift : public FOnlineAsyncTaskBasic<class FOnlineSubsystemZomboy>
{

private:
	bool bTaskThreadInitialized;

PACKAGE_SCOPE:
	/** Hidden on purpose */
	FOnlineAsyncTaskGameLift() :
		FOnlineAsyncTaskBasic(NULL),
		bTaskThreadInitialized(false)
	{

	}


protected:
	//Must get called on task thread
	virtual void TaskThreadInit() { }
	virtual void FinishTaskThread(bool bSuccess)
	{
		bIsComplete = true;
		bWasSuccessful = bSuccess;
	}

public:
	FOnlineAsyncTaskGameLift(class FOnlineSubsystemZomboy* InZomboySubsystem) :
		FOnlineAsyncTaskBasic(InZomboySubsystem),
		bTaskThreadInitialized(false)
	{
	}

	virtual ~FOnlineAsyncTaskGameLift()
	{
	}

	/**
	* Give the async task time to do its work
	* Can only be called on the async task manager thread
	*/
	virtual void Tick() override
	{
		if (!bTaskThreadInitialized)
		{
			bTaskThreadInitialized = true;
			TaskThreadInit();
		}
	}
};
#endif

#if WITH_STEAM

/**
* Base class that holds a delegate to fire when a given async task is complete
*/
class FOnlineAsyncTaskSteam : public FOnlineAsyncTaskBasic<class FOnlineSubsystemZomboy>
{
PACKAGE_SCOPE:

	/** Unique handle for the Steam async call initiated */
	SteamAPICall_t CallbackHandle;

	/** Hidden on purpose */
	FOnlineAsyncTaskSteam() :
		FOnlineAsyncTaskBasic(NULL),
		CallbackHandle(k_uAPICallInvalid)
	{
	}

public:

	FOnlineAsyncTaskSteam(class FOnlineSubsystemZomboy* InZomboySubsystem, SteamAPICall_t InCallbackHandle) :
		FOnlineAsyncTaskBasic(InZomboySubsystem),
		CallbackHandle(InCallbackHandle)
	{
	}

	virtual ~FOnlineAsyncTaskSteam()
	{
	}
};

#elif WITH_OCULUS
	
#include "OnlineMessageTaskManagerOculus.h"
typedef TUniquePtr<class FOnlineMessageTaskManagerOculus> FOnlineMessageTaskManagerOculusPtr;


class FOnlineAsyncTaskOculus : public FOnlineAsyncTaskBasic<class FOnlineSubsystemZomboy>
{

private:
	bool bTaskThreadInitialized;

PACKAGE_SCOPE:
	/** Hidden on purpose */
	FOnlineAsyncTaskOculus() :
		FOnlineAsyncTaskBasic(NULL),
		bTaskThreadInitialized(false)
	{

	}

protected:
	//Must get called on task thread
	virtual void TaskThreadInit() { }
	virtual void FinishTaskThread(bool bSuccess)
	{
		bIsComplete = true;
		bWasSuccessful = bSuccess;
	}

public:
	FOnlineAsyncTaskOculus(class FOnlineSubsystemZomboy* InZomboySubsystem) :
		FOnlineAsyncTaskBasic(InZomboySubsystem),
		bTaskThreadInitialized(false)
	{
	}

	virtual ~FOnlineAsyncTaskOculus()
	{
	}

	/**
	* Give the async task time to do its work
	* Can only be called on the async task manager thread
	*/
	virtual void Tick() override
	{
		if (!bTaskThreadInitialized)
		{
			bTaskThreadInitialized = true;
			TaskThreadInit();
		}
	}
};


#endif

/**
 *	Zomboy version of the async task manager to register the various Null callbacks with the engine
 */
class FOnlineAsyncTaskManagerZomboy : public FOnlineAsyncTaskManager
{
protected:

	/** Cached reference to the main online subsystem */
	class FOnlineSubsystemZomboy* ZomboySubsystem;

public:

	FOnlineAsyncTaskManagerZomboy(class FOnlineSubsystemZomboy* InOnlineSubsystem)
		: ZomboySubsystem(InOnlineSubsystem)
	{
	}

	~FOnlineAsyncTaskManagerZomboy()
	{
#if WITH_OCULUS

		if (MessageTaskManager.IsValid())
		{
			MessageTaskManager.Release();
		}
#endif
	}

	virtual bool Init() override;

	// FOnlineAsyncTaskManager
	virtual void OnlineTick() override;


#if WITH_OCULUS

private:
	/** Message Task Manager */
	FOnlineMessageTaskManagerOculusPtr MessageTaskManager;

public:
	/**
	* Allows for the LibOVRPlatform calls to be used directly with the Delegates in the Oculus OSS
	*/
	void AddRequestDelegate(ovrRequest RequestId, FOculusMessageOnCompleteDelegate&& Delegate) const;

	/**
	* Allows for direct subscription to the LibOVRPlatform notifications with the Delegates in the Oculus OSS
	*/
	FOculusMulticastMessageOnCompleteDelegate& GetNotifDelegate(ovrMessageType MessageType) const;
	void RemoveNotifDelegate(ovrMessageType MessageType, const FDelegateHandle& Delegate) const;

#endif
};

#if WITH_GAMELIFT_SERVER

typedef TSharedPtr<FGameLiftServerSDKModule, ESPMode::ThreadSafe> FGameLiftServerPtr;

/**
* Base class that holds a delegate to fire when a given async task is complete
*/
class FOnlineAsyncTaskGameLiftServer : public FOnlineAsyncTaskBasic<class FOnlineSubsystemZomboy>
{
private:
	bool bTaskThreadInitialized;

PACKAGE_SCOPE:
	/** Hidden on purpose */
	FOnlineAsyncTaskGameLiftServer() :
		FOnlineAsyncTaskBasic(NULL),
		bTaskThreadInitialized(false),
		GameLiftServer(NULL)
	{

	}


protected:

	FGameLiftServerPtr GameLiftServer;

	//Must get called on task thread
	virtual void TaskThreadInit() { }
	virtual void FinishTaskThread(bool bSuccess)
	{
		bIsComplete = true;
		bWasSuccessful = bSuccess;
	}

public:
	FOnlineAsyncTaskGameLiftServer(class FOnlineSubsystemZomboy* InZomboySubsystem, const FGameLiftServerPtr& InGameLiftServerModule) :
		FOnlineAsyncTaskBasic(InZomboySubsystem),
		bTaskThreadInitialized(false),
		GameLiftServer(InGameLiftServerModule)
	{
	}

	virtual ~FOnlineAsyncTaskGameLiftServer()
	{
	}

	/**
	* Give the async task time to do its work
	* Can only be called on the async task manager thread
	*/
	virtual void Tick() override
	{
		if (!bTaskThreadInitialized)
		{
			bTaskThreadInitialized = true;
			TaskThreadInit();
		}
	}
};

enum class EServerBackfillRequestType : uint8
{
	EServerBackfillRequestType_Start,
	EServerBackfillRequestType_Cancel
};

class FOnlineAsyncTaskGameLiftServerBackfill : public FOnlineAsyncTaskGameLiftServer
{
public:
	EServerBackfillRequestType Type;

private:

	/** Hidden on purpose */
	FOnlineAsyncTaskGameLiftServerBackfill() :
		FOnlineAsyncTaskGameLiftServer(NULL, NULL),
		Type(EServerBackfillRequestType::EServerBackfillRequestType_Start)
	{

	}

public:
	FOnlineAsyncTaskGameLiftServerBackfill(class FOnlineSubsystemZomboy* InSubsystem, const FGameLiftServerPtr& InGameLiftServerModule) :
		FOnlineAsyncTaskGameLiftServer(InSubsystem, InGameLiftServerModule),
		Type(EServerBackfillRequestType::EServerBackfillRequestType_Start)
	{

	}

};

class FOnlineAsyncTaskGameLiftServerBackfillStart : public FOnlineAsyncTaskGameLiftServerBackfill
{
private:
	FString TicketId;
	FString SessionId;
	FString MatchmakingConfigArn;
	TArray<FPlayer> MatchmakingPlayers;
	int MaxPlayers;

	/** Hidden on purpose */
	FOnlineAsyncTaskGameLiftServerBackfillStart() :
		FOnlineAsyncTaskGameLiftServerBackfill(NULL, NULL),
		SessionId(FString()),
		MatchmakingConfigArn(FString())
	{
		Type = EServerBackfillRequestType::EServerBackfillRequestType_Start;
	}

public:
	FOnlineAsyncTaskGameLiftServerBackfillStart(class FOnlineSubsystemZomboy* InSubsystem, const FGameLiftServerPtr& InGameLiftServerModule, const FString& InTicketId, const FString& InSessionId, const FString& InMatchmakingConfigArn, const TArray<FPlayer>& InMatchmakingPlayers, int InMaxPlayers) :
		FOnlineAsyncTaskGameLiftServerBackfill(InSubsystem, InGameLiftServerModule),
		TicketId(InTicketId),
		SessionId(InSessionId),
		MatchmakingConfigArn(InMatchmakingConfigArn),
		MatchmakingPlayers(InMatchmakingPlayers),
		MaxPlayers(InMaxPlayers)
	{
		Type = EServerBackfillRequestType::EServerBackfillRequestType_Start;
	}

	virtual FString ToString() const override;

	virtual void TaskThreadInit() override;
};

class FOnlineAsyncTaskGameLiftServerBackfillStop : public FOnlineAsyncTaskGameLiftServerBackfill
{
private:
	FString TicketId;
	FString SessionId;
	FString MatchmakingConfigArn;

	/** Hidden on purpose */
	FOnlineAsyncTaskGameLiftServerBackfillStop() :
		FOnlineAsyncTaskGameLiftServerBackfill(NULL, NULL),
		SessionId(FString()),
		MatchmakingConfigArn(FString())
	{
		Type = EServerBackfillRequestType::EServerBackfillRequestType_Start;
	}

public:
	FOnlineAsyncTaskGameLiftServerBackfillStop(class FOnlineSubsystemZomboy* InSubsystem, const FGameLiftServerPtr& InGameLiftServerModule, const FString& InTicketId, const FString& InSessionId, const FString& InMatchmakingConfigArn) :
		FOnlineAsyncTaskGameLiftServerBackfill(InSubsystem, InGameLiftServerModule),
		TicketId(InTicketId),
		SessionId(InSessionId),
		MatchmakingConfigArn(InMatchmakingConfigArn)
	{
		Type = EServerBackfillRequestType::EServerBackfillRequestType_Cancel;
	}

	virtual FString ToString() const override;

	virtual void TaskThreadInit() override;
};

class FOnlineAsyncTaskGameLiftServerRemovePlayerSession : public FOnlineAsyncTaskGameLiftServer
{
private:
	FString PlayerSessionId;

	FOnlineAsyncTaskGameLiftServerRemovePlayerSession() :
		FOnlineAsyncTaskGameLiftServer(NULL, NULL),
		PlayerSessionId(FString())
	{

	}

public:
	FOnlineAsyncTaskGameLiftServerRemovePlayerSession(class FOnlineSubsystemZomboy* InSubsystem, const FGameLiftServerPtr& InGameLiftServerModule, const FString& InPlayerSessionId) :
		FOnlineAsyncTaskGameLiftServer(InSubsystem, InGameLiftServerModule),
		PlayerSessionId(InPlayerSessionId)
	{

	}

	virtual FString ToString() const override;

	virtual void TaskThreadInit() override;
};


/**
 *	Zomboy version of the async task manager to register the various server callbacks with the engine
 */
class FOnlineAsyncTaskManagerZomboyServer : public FOnlineAsyncTaskManager
{
protected:

	/** Cached reference to the main online subsystem */
	class FOnlineSubsystemZomboy* ZomboySubsystem;

public:

	FOnlineAsyncTaskManagerZomboyServer(class FOnlineSubsystemZomboy* InOnlineSubsystem)
		: ZomboySubsystem(InOnlineSubsystem),
		ActiveBackfillTask(nullptr)
	{
	}

	~FOnlineAsyncTaskManagerZomboyServer()
	{

	}

	void AddBackfillStartRequest(const FGameLiftServerPtr& GameLiftServer, const FString& SessionId, const FString& MatchmakingConfigArn, const TArray<FPlayer>& MatchmakingPlayers, int MaxPlayers);
	void AddBackfillStopRequest(const FGameLiftServerPtr& GameLiftServer, const FString& SessionId, const FString& MatchmakingConfigArn);
	void AddRemovePlayerSessionRequest(const FGameLiftServerPtr& GameLiftServer, const FString& PlayerSessionId);

protected:

	TArray<FOnlineAsyncTaskGameLiftServerBackfill*> InBackfillTaskQueue;
	FCriticalSection InBackfillTaskQueueLock;

	FOnlineAsyncTaskGameLiftServerBackfill* ActiveBackfillTask;
	FCriticalSection ActiveBackfillTaskLock;

	FString CurrentBackfillTicketId;

public:

	// FOnlineAsyncTaskManager
	virtual void OnlineTick() override
	{

	}

	void GameServerTick();
	virtual void Tick() override;

};


#endif //WITH_GAMELIFT_SERVER