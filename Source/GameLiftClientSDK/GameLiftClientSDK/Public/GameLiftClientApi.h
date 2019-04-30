// Created by YetiTech Studios.

#pragma once

#include "CoreMinimal.h"
#include "GameLiftClientTypes.h"
#include "OnlineSubsystemUtils.h"

// AWS includes
#include "aws/gamelift/GameLiftClient.h"
#include "aws/gamelift/model/CreateGameSessionResult.h"
#include "aws/core/auth/AWSCredentialsProvider.h"
//

#include "GameLiftClientApi.generated.h"

UENUM(BlueprintType)
enum class EActivateStatus : uint8
{
	/* Successfully activated. */
	ACTIVATE_Success				UMETA(DisplayName = "Success"),

	/* GameLiftClient was null. Make sure you called CreateGameLiftObject function. */
	ACTIVATE_NoGameLift				UMETA(DisplayName = "Null GameLift"),

	/* Delegate that was suppose to call when outcome is a success was not binded to any function. */
	ACTIVATE_NoSuccessCallback		UMETA(DisplayName = "Success Delegate not bound"),

	/* Delegate that was suppose to call when outcome is a failure was not binded to any function. */
	ACTIVATE_NoFailCallback			UMETA(DisplayName = "Failed Delegate not bound")
};

DECLARE_DELEGATE_OneParam(FOnPlaceGameSessionInQueueSuccessDelegate, bool);
DECLARE_DELEGATE_OneParam(FOnPlaceGameSessionInQueueFailedDelegate, const FString&);

class GAMELIFTCLIENTSDK_API FGameLiftPlaceGameSessionInQueue : public TSharedFromThis<FGameLiftPlaceGameSessionInQueue, ESPMode::ThreadSafe>
{
	friend class FGameLiftClientObject;

public:
	FOnPlaceGameSessionInQueueSuccessDelegate OnPlaceGameSessionSuccessDelegate;
	FOnPlaceGameSessionInQueueFailedDelegate OnPlaceGameSessionFailedDelegate;

private:
	Aws::GameLift::GameLiftClient* GameLiftClient;
	FGameLiftGameSessionConfig SessionConfig;
	FString QueueName;
	FString UniqueId;
	TArray<TSharedRef<const FUniqueNetId>> PlayerIds;

	static TSharedPtr<FGameLiftPlaceGameSessionInQueue, ESPMode::ThreadSafe> PlaceGameSessionInQueue(FGameLiftGameSessionConfig GameSessionProperties, const FString& QueueName, const FString& UniqueId, const TArray<TSharedRef<const FUniqueNetId>>& PlayerIds);

public:
	EActivateStatus Activate();

private:
	void OnPlacedGameSessionComplete(const Aws::GameLift::GameLiftClient* Client, const Aws::GameLift::Model::StartGameSessionPlacementRequest& Request, const Aws::GameLift::Model::StartGameSessionPlacementOutcome& Outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& Context);
};


DECLARE_DELEGATE_FiveParams(FOnDescribeGameLiftGameSessionPlacementSuccessDelegate, Aws::GameLift::Model::GameSessionPlacementState, const FString& /*SessionId*/, const FString& /*IPAddress*/, const FString& /*Port*/, const Aws::Vector<Aws::GameLift::Model::PlacedPlayerSession>&/*PlacedPlayerSessions*/);
DECLARE_DELEGATE_OneParam(FOnDescribeGameLiftGameSessionPlacementFailedDelegate, const FString&);

class GAMELIFTCLIENTSDK_API FDescribeGameLiftGameSessionPlacement : public TSharedFromThis<FDescribeGameLiftGameSessionPlacement, ESPMode::ThreadSafe>
{
	friend class FGameLiftClientObject;

public:
	FOnDescribeGameLiftGameSessionPlacementSuccessDelegate OnDescribeGameLiftGameSessionPlacementSuccessDelegate;
	FOnDescribeGameLiftGameSessionPlacementFailedDelegate OnDescribeGameLiftGameSessionPlacementFailedDelegate;

private:
	Aws::GameLift::GameLiftClient* GameLiftClient;
	FString UniqueId;

	static TSharedPtr<FDescribeGameLiftGameSessionPlacement, ESPMode::ThreadSafe> DescribeGameSessionPlacement(const FString& UniqueId);

public:
	EActivateStatus Activate();

private:
	void OnDescribeSessionPlacement(const Aws::GameLift::GameLiftClient* Client, const Aws::GameLift::Model::DescribeGameSessionPlacementRequest& Request, const Aws::GameLift::Model::DescribeGameSessionPlacementOutcome& Outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& Context);
};


DECLARE_DELEGATE_TwoParams(FOnStartMatchmakingSuccessDelegate, Aws::GameLift::Model::MatchmakingConfigurationStatus, const FString&);
DECLARE_DELEGATE_OneParam(FOnStartMatchmakingFailedDelegate, const FString&);

class GAMELIFTCLIENTSDK_API FStartGameLiftMatchmaking : public TSharedFromThis<FStartGameLiftMatchmaking, ESPMode::ThreadSafe>
{
	friend class FGameLiftClientObject;

public:
	FOnStartMatchmakingSuccessDelegate OnStartMatchmakingSuccessDelegate;
	FOnStartMatchmakingFailedDelegate OnStartMatchmakingFailedDelegate;

private:
	Aws::GameLift::GameLiftClient* GameLiftClient;
	FString ConfigName;
	TArray<TSharedRef<const FUniqueNetId>> PlayerIds;
	Aws::Map<Aws::String, int> RegionLatency;
	FString MatchmakingTicketId;
	//TArray<TSharedPtr<FGameLiftPlayerAttribute>> PlayerAttributes;
	TMap<FName, FClientAttributeValue> PlayerAttributes;

	static TSharedPtr<FStartGameLiftMatchmaking, ESPMode::ThreadSafe> StartMatchmaking(const FString& ConfigName, const TArray<TSharedRef<const FUniqueNetId>>& PlayerIds, FString MatchmakingTicketId, const TMap<FName, FClientAttributeValue>& PlayerAttributes);

public:
	void ChangeMatchmakingTicket(const FString& NewMatchmakingTicketId);
	EActivateStatus Activate();

private:
	void OnGameLiftMatchmakingComplete(const Aws::GameLift::GameLiftClient* Client, const Aws::GameLift::Model::StartMatchmakingRequest& Request, const Aws::GameLift::Model::StartMatchmakingOutcome& Outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& Context);
};

DECLARE_DELEGATE_SixParams(FOnDescribeMatchmakingSuccessDelegate, Aws::GameLift::Model::MatchmakingConfigurationStatus, const FString& /*Session ID*/, const FString& /*IP Address*/, const FString& /*Port*/, const Aws::Vector<Aws::GameLift::Model::MatchedPlayerSession>& /*Matched Player Session*/, const Aws::Vector<Aws::GameLift::Model::Player>& /*Aws Player*/);
DECLARE_DELEGATE_ThreeParams(FOnDescribeMatchmakingFailedDelegate, const FString&, Aws::GameLift::Model::MatchmakingConfigurationStatus, bool /*bCriticalFailure*/);

class GAMELIFTCLIENTSDK_API FDescribeGameLiftMatchmaking : public TSharedFromThis<FDescribeGameLiftMatchmaking, ESPMode::ThreadSafe>
{
	friend class FGameLiftClientObject;
public:
	FOnDescribeMatchmakingSuccessDelegate OnDescribeMatchmakingSuccessDelegate;
	FOnDescribeMatchmakingFailedDelegate OnDescribeMatchmakingFailedDelegate;

private:
	Aws::GameLift::GameLiftClient* GameLiftClient;
	FString TickId;

	static TSharedPtr<FDescribeGameLiftMatchmaking, ESPMode::ThreadSafe> DescribeMatchmaking(const FString& TickId);

public:
	EActivateStatus Activate();

private:
	void OnDescribeMatchmakingComplete(const  Aws::GameLift::GameLiftClient* Client, const  Aws::GameLift::Model::DescribeMatchmakingRequest& Request, const  Aws::GameLift::Model::DescribeMatchmakingOutcome& Outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& Context);
};

DECLARE_DELEGATE(FOnStopGameLiftMatchmakingSuccessDelegate);
DECLARE_DELEGATE(FOnStopGameLiftMatchmakingFailedDelegate);

class GAMELIFTCLIENTSDK_API FStopGameLiftMatchmaking : public TSharedFromThis<FStopGameLiftMatchmaking, ESPMode::ThreadSafe>
{
	friend class FGameLiftClientObject;
public:
	FOnStopGameLiftMatchmakingSuccessDelegate OnStopGameLiftMatchmakingSuccessDelegate;
	FOnStopGameLiftMatchmakingFailedDelegate OnStopGameLiftMatchmakingFailedDelegate;

private:
	Aws::GameLift::GameLiftClient* GameLiftClient;
	FString TickId;

	static TSharedPtr<FStopGameLiftMatchmaking, ESPMode::ThreadSafe> StopMatchmaking(const FString& TickId);

public:
	EActivateStatus Activate();

private:
	void OnStopMatchmakingComplete(const  Aws::GameLift::GameLiftClient* Client, const  Aws::GameLift::Model::StopMatchmakingRequest& Request, const  Aws::GameLift::Model::StopMatchmakingOutcome& Outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& Context);
};


DECLARE_DELEGATE_OneParam(FOnSearchGameSessionsSuccessDelegate, const TArray<FGameLiftGameSessionSearchResult>&);
DECLARE_DELEGATE_OneParam(FOnSearchGameSessionsFailedDelegate, const FString&);

class GAMELIFTCLIENTSDK_API FGameLiftSearchGameSessions : public TSharedFromThis<FGameLiftSearchGameSessions, ESPMode::ThreadSafe>
{
	friend class FGameLiftClientObject;
public:
	FOnSearchGameSessionsSuccessDelegate OnSearchGameSessionsSuccessDelegate;
	FOnSearchGameSessionsFailedDelegate OnSearchGameSessionsFailedDelegate;

private:
	Aws::GameLift::GameLiftClient* GameLiftClient;
	Aws::Map<Aws::String, Aws::GameLift::GameLiftClient*> GameLiftClientMap;

	FGameLiftGameSessionSearchSettings SearchSettings;
	bool bIsUsingGameLiftLocal;

	static TSharedPtr<FGameLiftSearchGameSessions, ESPMode::ThreadSafe> SearchGameSessions(FGameLiftGameSessionSearchSettings SearchSettings, bool bIsGameLiftLocal);

public:
	UFUNCTION(BlueprintCallable, Category = "GameLift SearchGameSession")
	EActivateStatus Activate();
};


DECLARE_DELEGATE_TwoParams(FOnDescribeGameSessionStateSuccessDelegate, const FString&, EGameLiftGameSessionStatus);
DECLARE_DELEGATE_OneParam(FOnDescribeGameSessionStateFailedDelegate, const FString&);

class GAMELIFTCLIENTSDK_API FGameLiftDescribeGameSession : public TSharedFromThis<FGameLiftDescribeGameSession, ESPMode::ThreadSafe>
{
	friend class FGameLiftClientObject;

public:

	FOnDescribeGameSessionStateSuccessDelegate OnDescribeGameSessionStateSuccessDelegate;
	
	FOnDescribeGameSessionStateFailedDelegate OnDescribeGameSessionStateFailedDelegate;

private:
	Aws::GameLift::GameLiftClient* GameLiftClient;
	FString SessionID;

	static TSharedPtr<FGameLiftDescribeGameSession, ESPMode::ThreadSafe> DescribeGameSession(FString GameSessionID);

public:
	UFUNCTION(BlueprintCallable, Category = "GameLift DescribeGameSession")
	EActivateStatus Activate();	

private:
	void OnDescribeGameSessionState(const Aws::GameLift::GameLiftClient* Client, const Aws::GameLift::Model::DescribeGameSessionDetailsRequest& Request, const Aws::GameLift::Model::DescribeGameSessionDetailsOutcome& Outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& Context);
	EGameLiftGameSessionStatus GetSessionState(const Aws::GameLift::Model::GameSessionStatus& Status);
};

DECLARE_DELEGATE_ThreeParams(FOnCreatePlayerSessionSuccessDelegate, const FString& /*IPAddress*/, const FString& /*Port*/, const FString& /*PlayerSessionID*/);
DECLARE_DELEGATE_OneParam(FOnCreatePlayerSessionFailedDelegate, const FString& /*ErrorMessage*/);

class GAMELIFTCLIENTSDK_API FGameLiftCreatePlayerSession : public TSharedFromThis<FGameLiftCreatePlayerSession, ESPMode::ThreadSafe>
{
	friend class FGameLiftClientObject;

public:

	FOnCreatePlayerSessionSuccessDelegate OnCreatePlayerSessionSuccessDelegate;
	FOnCreatePlayerSessionFailedDelegate OnCreatePlayerSessionFailedDelegate;

private:
	Aws::GameLift::GameLiftClient* GameLiftClient;

	FString GameSessionID;
	FString PlayerID;
	EGameLiftRegion GameLiftRegion;

	static TSharedPtr<FGameLiftCreatePlayerSession, ESPMode::ThreadSafe> CreatePlayerSession(FString GameSessionID, FString UniquePlayerID, EGameLiftRegion GameLiftRegion);

public:
	EActivateStatus Activate();

private:
	void OnCreatePlayerSession(const Aws::GameLift::GameLiftClient* Client, const Aws::GameLift::Model::CreatePlayerSessionRequest& Request, const Aws::GameLift::Model::CreatePlayerSessionOutcome& Outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& Context);
};
