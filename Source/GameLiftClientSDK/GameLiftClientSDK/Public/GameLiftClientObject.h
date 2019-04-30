// Created by YetiTech Studios.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameLiftClientTypes.h"



class GAMELIFTCLIENTSDK_API FGameLiftClientObject : public TSharedFromThis<FGameLiftClientObject, ESPMode::ThreadSafe>
{

private:

#if WITH_GAMELIFTCLIENTSDK	
	Aws::GameLift::GameLiftClient* GameLiftClient;
	Aws::Map<Aws::String, Aws::GameLift::GameLiftClient*> GameLiftClientMap;
#endif

	EGameLiftRegion CurrentRegion;

	bool bIsUsingGameLiftLocal;

	void Internal_InitGameLiftClientSDK(const FString& AccessKey, const FString& Secret, EGameLiftRegion Region, bool bUsingGameLiftLocal, int32 LocalPort);
	
public:
	
	EGameLiftRegion GetCurrentRegion();

	bool SetCurrentRegion(EGameLiftRegion Region);

	Aws::GameLift::GameLiftClient* GetClientFromRegion(EGameLiftRegion Region);

	void UpdateGameSession(FString SessionId, FString SessionName);

	/**
	* public static FGameLiftClientObject::CreateGameLiftObject
	* Creates a GameLiftClientObject. This function must be called first before accessing any GameLift client functions.
	* @param AccessKey [const FString&] AccessKey of your AWS user. @See http://docs.aws.amazon.com/general/latest/gr/managing-aws-access-keys.html
	* @param Secret [const FString&] SecretKey of your AWS user. @See http://docs.aws.amazon.com/general/latest/gr/managing-aws-access-keys.html
	* @param Region [const FString&] Default is set to us-east-1 (North Virginia).
	* @param bUsingGameLiftLocal [bool] If true, then it is assumed you are using GameLift Local. @See http://docs.aws.amazon.com/gamelift/latest/developerguide/integration-testing-local.html#integration-testing-local-server
	* @param LocalPort [int32] port to use if bUsingGameLiftLocal is true.
	* @return [FGameLiftClientObject*] Returns FGameLiftClientObject*. Use this to create game sessions, player sessions etc.
	**/
	static TSharedPtr<FGameLiftClientObject, ESPMode::ThreadSafe> CreateGameLiftObject(const FString& AccessKey, const FString& Secret, EGameLiftRegion Region = EGameLiftRegion::EGameLiftRegion_APJ, bool bUsingGameLiftLocal = false, int32 LocalPort = 9080);
	

	TSharedPtr<class FGameLiftPlaceGameSessionInQueue, ESPMode::ThreadSafe> PlaceGameSessionInQueue(FGameLiftGameSessionConfig GameSessionProperties, const FString& QueueName, const FString& UniqueId, const TArray<TSharedRef<const FUniqueNetId>>& PlayerIds);

	TSharedPtr<class FDescribeGameLiftGameSessionPlacement, ESPMode::ThreadSafe> DescribeGameSessionPlacement(const FString& UniqueId);

	TSharedPtr<class FStartGameLiftMatchmaking, ESPMode::ThreadSafe> StartMatchmaking(const FString& ConfigName, const TArray<TSharedRef<const FUniqueNetId>>& PlayerIds, const FString& MatchmakingTicket, const TMap<FName, FClientAttributeValue>& PlayerAttributes);

	TSharedPtr<class FStopGameLiftMatchmaking, ESPMode::ThreadSafe> StopMatchmaking(const FString& MatchmakingTicket);

	TSharedPtr<class FDescribeGameLiftMatchmaking, ESPMode::ThreadSafe> DescribeMatchmaking(const FString& TickId);

	TSharedPtr<class FGameLiftSearchGameSessions, ESPMode::ThreadSafe> SearchGameLiftSessions(FGameLiftGameSessionSearchSettings GameSessionSearchSettings);

	/**
	* public FGameLiftClientObject::DescribeGameSession
	* Retrieves the given game session.
	* From the return value first bind both success and fail events and then call Activate to describe game session.
	* @param GameSessionID [FString] Game Session ID. This can be obtained after a successful create game session.
	* @return [FGameLiftDescribeGameSession*] Returns FGameLiftDescribeGameSession* Object.
	**/
	TSharedPtr<class FGameLiftDescribeGameSession, ESPMode::ThreadSafe> DescribeGameSession(FString GameSessionID);

	/**
	* public FGameLiftClientObject::CreatePlayerSession
	* Adds a player to a game session and creates a player session record. 
	* Before a player can be added, a game session must have an ACTIVE status, have a creation policy of ALLOW_ALL, and have an open player slot.	
	* From the return value first bind both success and fail events and then call Activate to create player session.
	* @param GameSessionID [FString] Unique identifier for the game session to add a player to.
	* @param UniquePlayerID [FString] Unique identifier for a player. Player IDs are developer-defined.
	* @return [FGameLiftCreatePlayerSession*] Returns FGameLiftCreatePlayerSession* Object.
	**/
	TSharedPtr<class FGameLiftCreatePlayerSession, ESPMode::ThreadSafe> CreatePlayerSession(FString GameSessionID, FString UniquePlayerID, EGameLiftRegion GameLiftRegion);
};
