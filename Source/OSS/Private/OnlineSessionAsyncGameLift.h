#pragma once

#include "CoreMinimal.h"
#include "UObject/CoreOnline.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineAsyncTaskManager.h"
#include "OnlineAsyncTaskManagerZomboy.h"
#include "OnlineSubsystemZomboyPackage.h"

#if WITH_GAMELIFTCLIENTSDK

class FOnlineAsyncTaskCreateGameliftSession : public FOnlineAsyncTaskGameLift
{
private:
	/** Hidden on purpose */
	FOnlineAsyncTaskCreateGameliftSession() :
		GameLiftObject(NULL),
		SessionName(NAME_None),
		QuerySessionTimeStamp(0)
	{

	}

public:
	FOnlineAsyncTaskCreateGameliftSession(class FOnlineSubsystemZomboy* InSubsystem, FName InSessionName, TArray<TSharedRef<const FUniqueNetId>> InPlayerIds) :
		FOnlineAsyncTaskGameLift(InSubsystem),
		SessionName(InSessionName),
		PlayerIds(InPlayerIds),
		GameLiftObject(NULL),
		QuerySessionTimeStamp(0)
	{

	}

protected:

	/** Current GameLift Object */
	TSharedPtr<FGameLiftClientObject, ESPMode::ThreadSafe> GameLiftObject;

	/** GameLift Start Game Session Placement Objecct*/
	TSharedPtr<FGameLiftPlaceGameSessionInQueue, ESPMode::ThreadSafe> PlaceGameSessionObject;
	/** GameLift Describe Game Session Placement Objecct*/
	TSharedPtr<FDescribeGameLiftGameSessionPlacement, ESPMode::ThreadSafe> DescribeGameSessionPlacementObject;

	TArray<TSharedRef<const FUniqueNetId>> PlayerIds;

	/** Name of session being created */
	FName SessionName;

	FString SessionPlacementUniqueId;

	float QuerySessionTimeStamp;

	FString GameLiftSessionId;
	FString GameLiftServerIp;
	FString GameLiftServerPort;
	FString GameLiftPlayerSessionId;

protected:

	virtual FString ToString() const override;

	virtual void TaskThreadInit() override;

	void OnPlaceGameLiftGameSessionSuccess(bool bPlacementSuccessful);
	void OnPlaceGameLiftGameSessionFailed(const FString& ErroeMessage);

	void OnDescribeGameSessionPlacementSuccess(Aws::GameLift::Model::GameSessionPlacementState Status, const FString& SessionId, const FString& IPAddress, const FString& Port, const Aws::Vector<Aws::GameLift::Model::PlacedPlayerSession>& PlacedPlayerSessions);
	void OnDescribeGameSessionPlacementFailed(const FString& ErrorMsg);

	virtual void Finalize() override;

	virtual void TriggerDelegates() override;
};

class FOnlineAsyncTaskSearchGameliftSession : public FOnlineAsyncTaskGameLift
{
private:
	/** Hidden on purpose */
	FOnlineAsyncTaskSearchGameliftSession() : 
		GameLiftObject(NULL),
		SearchSettings(NULL),
		SearchGameSessionsObject(NULL)
	{

	}

public:
	FOnlineAsyncTaskSearchGameliftSession(class FOnlineSubsystemZomboy* InSubsystem, const TSharedPtr<class FOnlineSessionSearch>& InSearchSettings) :
		FOnlineAsyncTaskGameLift(InSubsystem),
		GameLiftObject(NULL),
		SearchSettings(InSearchSettings),
		SearchGameSessionsObject(NULL)
	{

	}

protected:

	TSharedPtr<class FOnlineSessionSearch> SearchSettings;

	/** Current GameLift Object */
	TSharedPtr<FGameLiftClientObject, ESPMode::ThreadSafe> GameLiftObject;

	/** GameLift Search Game Session Objecct*/
	TSharedPtr<FGameLiftSearchGameSessions, ESPMode::ThreadSafe> SearchGameSessionsObject;

protected:
	virtual FString ToString() const override;

	virtual void TaskThreadInit() override;

	void OnSearchGameSessionsSuccess(const TArray<FGameLiftGameSessionSearchResult>& Results);
	void OnSearchGameSessionsFailed(const FString& ErrorMessage);

	virtual void Finalize() override;
	virtual void TriggerDelegates() override;
};

class FOnlineAsyncTaskJoinGameliftSession : public FOnlineAsyncTaskGameLift
{
private:
	/** Name of session lobby */
	FName SessionName;
	/** LobbyId to end */
	FUniqueNetIdString GameSessionId;
	/** Region*/
	EGameLiftRegion GameSessionRegion;

	/** Hidden on purpose */
	FOnlineAsyncTaskJoinGameliftSession() :
		GameLiftObject(NULL),
		CreatePlayerSessionObject(NULL),
		GameSessionRegion(EGameLiftRegion::EGameLiftRegion_None)
	{

	}

protected:
	/** Current GameLift Object */
	TSharedPtr<FGameLiftClientObject, ESPMode::ThreadSafe> GameLiftObject;

	/** GameLift Create Player Session Objecct*/
	TSharedPtr<FGameLiftCreatePlayerSession, ESPMode::ThreadSafe> CreatePlayerSessionObject;

public:
	FOnlineAsyncTaskJoinGameliftSession(class FOnlineSubsystemZomboy* InSubsystem, FName InSessionName, const FUniqueNetIdString& InGameSessoinId, EGameLiftRegion InGameSessionRegion) :
		FOnlineAsyncTaskGameLift(InSubsystem),
		SessionName(InSessionName),
		GameSessionId(InGameSessoinId),
		GameSessionRegion(InGameSessionRegion)
	{

	}

	/**
	*	Get a human readable description of task
	*/
	virtual FString ToString() const override;

	virtual void TaskThreadInit() override;

	void OnCreatePlayerSessionSuccess(const FString& IPAddress, const FString& Port, const FString& PlayerSessionID);
	void OnCreatePlayerSessionFailed(const FString& ErrorMessage);

	virtual void Finalize() override;
	virtual void TriggerDelegates() override;

};

class FOnlineAsyncTaskStartGameliftMatchmaking : public FOnlineAsyncTaskGameLift
{
private:
	/** Name of session lobby */
	FName SessionName;
	const TArray<TSharedRef<const FUniqueNetId>> PlayerIds;
	const TSharedRef<FOnlineSessionSearchZomboy> MatchmakingSettings;
	FString GameSessionId;
	FString MatchmakingTicket;
	bool bMatchAccepted;

	/** Hidden on purpose */
	FOnlineAsyncTaskStartGameliftMatchmaking() :
		GameLiftObject(NULL),
		SessionName(FName()),
		MatchmakingTicket(FString()),
		GameSessionId(FString()),
		StartMatchmakingObject(NULL),
		DescribeMatchmakingObject(NULL)
	{

	}

protected:
	/** Current GameLift Object */
	TSharedPtr<FGameLiftClientObject, ESPMode::ThreadSafe> GameLiftObject;

	/** GameLift Start Matchmaking Objecct*/
	TSharedPtr<FStartGameLiftMatchmaking, ESPMode::ThreadSafe> StartMatchmakingObject;

	/** GameLift Describe Matchmaking Objecct*/
	TSharedPtr<FDescribeGameLiftMatchmaking, ESPMode::ThreadSafe> DescribeMatchmakingObject;

	float QuerySessionTimeStamp = 0;

public:
	FOnlineAsyncTaskStartGameliftMatchmaking(class FOnlineSubsystemZomboy* InSubsystem, FName InSessionName, const TArray<TSharedRef<const FUniqueNetId>>& InPlayerIds, const TSharedRef<FOnlineSessionSearchZomboy>& InMatchmakingSettings) :
		FOnlineAsyncTaskGameLift(InSubsystem),
		GameLiftObject(NULL),
		SessionName(InSessionName),
		MatchmakingTicket(FString()),
		GameSessionId(FString()),
		PlayerIds(InPlayerIds),
		MatchmakingSettings(InMatchmakingSettings),
		StartMatchmakingObject(NULL),
		DescribeMatchmakingObject(NULL)
	{

	}
	/**
	*	Get a human readable description of task
	*/
	virtual FString ToString() const override;

	virtual void TaskThreadInit() override;

	void OnStartMatchmakingSuccess(Aws::GameLift::Model::MatchmakingConfigurationStatus Status, const FString& MatchmakingTicket);
	void OnStartMatchmakingFailed(const FString& ErrorMessage);

	void OnDescribeMatchmakingSuccess(Aws::GameLift::Model::MatchmakingConfigurationStatus Status, const FString& InGameSessionId, const FString& IPAddress, const FString& Port, const Aws::Vector<Aws::GameLift::Model::MatchedPlayerSession>& MatchedPlayerSessions, const Aws::Vector<Aws::GameLift::Model::Player>& GameLiftPlayers);
	void OnDescribeMatchmakingFailed(const FString& ErrorMessage, Aws::GameLift::Model::MatchmakingConfigurationStatus Status, bool bCriticalFailure);

	virtual void Finalize() override;
	virtual void TriggerDelegates() override;
};

class FOnlineAsyncTaskStopGameliftMatchmaking : public FOnlineAsyncTaskGameLift
{
private:
	/** Name of session lobby */
	FName SessionName;

	/** Hidden on purpose */
	FOnlineAsyncTaskStopGameliftMatchmaking() :
		GameLiftObject(NULL),
		SessionName(FName())
	{

	}

protected:
	/** Current GameLift Object */
	TSharedPtr<FGameLiftClientObject, ESPMode::ThreadSafe> GameLiftObject;

	/** GameLift Stop Matchmaking Objecct*/
	TSharedPtr<FStopGameLiftMatchmaking, ESPMode::ThreadSafe> StopMatchmakingObject;

public:
	FOnlineAsyncTaskStopGameliftMatchmaking(class FOnlineSubsystemZomboy* InSubsystem, FName InSessionName) :
		FOnlineAsyncTaskGameLift(InSubsystem),
		GameLiftObject(NULL),
		SessionName(InSessionName)
	{

	}

	/**
	*	Get a human readable description of task
	*/
	virtual FString ToString() const override;

	virtual void TaskThreadInit() override;

	void OnSotpMatchmakingSuccess();
	void OnSotpMatchmakingFailed();

	virtual void Finalize() override;
	virtual void TriggerDelegates() override;
};

#endif
