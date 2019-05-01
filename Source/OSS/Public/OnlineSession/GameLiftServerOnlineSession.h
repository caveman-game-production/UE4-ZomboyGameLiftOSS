// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/OnlineSession.h"
#include "Engine.h"

#if WITH_GAMELIFT_SERVER
#include "GameLiftServerSDK.h"
#endif

#include "GameLiftClientTypes.h"
#include "OnlineSubsystemTypes.h"
#include "OnlineSubsystemZomboyTypes.h"

#include "GameLiftServerOnlineSession.generated.h"

/**
 * 
 */
UCLASS(config = Game)
class ONLINESUBSYSTEMZOMBOY_API UGameLiftServerOnlineSession : public UOnlineSession
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static UGameLiftServerOnlineSession* GetServerOnlineSession(const UObject* WorldContextObject);

	template< class T>
	static T* GetServerOnlineSession(const UObject* WorldContextObject)
	{
		return Cast<T>(GetServerOnlineSession(WorldContextObject));
	}

protected:
	class UGameInstance * GetGameInstance() const;

	UWorld* GetWorld() const;

public:
	void InitGameLiftServices();

	void TerminateGameSession();

	// Should be called in AGameMode::PreLogin to register player with GameLift GameSession. 
	bool AcceptPlayerSession(const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage);
	// Should be called in AGameMode::Logout to remove player farom GameLift GameSession
	void RemovePlayerSession(const FUniqueNetIdRepl& UniqueId);

	void RequestBackfill(bool bFromFailure = false);

	void ServerChangeDynamicProperties(const TMap<FName, FString>& NewGameProperties);
	void ServerChangeMap(const TMap<FName, FString>& NewGameProperties);

	bool IsMatchmakingServer() { return CurrentGameLiftServerSession.bIsMatchmakingServer; }
	EGameLiftRegion GetServerRegion() { return CurrentGameLiftServerSession.Region; }
	FString GetServerDynamicProperty(const FName& PropertyKey) { return CurrentGameLiftServerSession.GetGameSessionDynamicProperty(PropertyKey); }
	FString GetServerStaticProperty(const FName& PropertyKey) { return CurrentGameLiftServerSession.GetGameSessionStaticProperty(PropertyKey); }
	FString GatServerMatchmakingData() { return CurrentGameLiftServerSession.MatchmakingData; }

public:
	TMap<FString/*TeamName*/, TArray<FUniqueNetIdZomboyPlayer>/*Player NetId Array*/> PlayerTeamMap;

#if WITH_GAMELIFT_SERVER

private:

	TSharedPtr<FGameLiftServerSDKModule, ESPMode::ThreadSafe> GameLiftSdkModule;

	void AsyncOnServiceActivate(Aws::GameLift::Server::Model::GameSession GameSession);
	void AsyncOnServiceUpdate(Aws::GameLift::Server::Model::UpdateGameSession UpdateGameSession);
	void AsyncOnServiceTerminate();
	bool AsyncOnServiceHealthCheck();

	void OnServiceReady(const FGameLiftServerGameSession& GameLiftGameSession);

#endif
	
protected:

	void SetServerRegion(const FString& FleetID);
	void BuildPlayerMatchmakingReservation(const FString& MatchmakingData);

	virtual void ProcessGameSessionReady(FGameLiftServerGameSession& GameLiftGameSession);
	virtual FString GetServerTravelURL(const FGameLiftServerGameSession& GameLiftGameSession);
	virtual void GetAdvertisedDynamicServerProperties(const FGameLiftServerGameSession& GameLiftGameSession, TMap<FName, FString>& OutPropertiesToAdvertise);

	virtual int GetPlayerTeamNumber(class APlayerState* PlayerState);
	virtual EGameLiftRegion GetPlayerRegion(class APlayerState* PlayerState);
	virtual TMap<FString, FClientAttributeValue> GetPlayerAttributes(class APlayerState* PlayerState);


	void TryStartMatchmakingBackfillRequest();
	void AdvertiseServerProperties();

public:

	UPROPERTY(config)
	FString DedicatedServerMap;

	bool bIsHostingLan = false;


private:

	bool bInitialized = false;

	FGameLiftServerGameSession CurrentGameLiftServerSession;

	/* Unique ID as a string to session id */
	TMap<FString, TArray<FString>> PlayerSessions;

	FTimerHandle TimerHandle_RequestBackfillTimer;
	bool bInBackfillFailure = false;

};
