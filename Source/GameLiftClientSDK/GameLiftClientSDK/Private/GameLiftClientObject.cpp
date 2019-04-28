// Created by YetiTech Studios.

#include "GameLiftClientObject.h"
#include "aws/core/client/ClientConfiguration.h"
#include "GameLiftClientApi.h"
#include "aws/gamelift/model/UpdateGameSessionRequest.h"

EGameLiftRegion FGameLiftClientObject::GetCurrentRegion()
{
	return CurrentRegion;
}

bool FGameLiftClientObject::SetCurrentRegion(EGameLiftRegion Region)
{
	Aws::String RegionString = TCHAR_TO_UTF8(GetRegionString(Region));
	auto RegionClient = GameLiftClientMap.find(RegionString);
	if (RegionClient != GameLiftClientMap.end())
	{
		CurrentRegion = Region;
		GameLiftClient = RegionClient->second;
		return true;
	}

	return false;
}

Aws::GameLift::GameLiftClient* FGameLiftClientObject::GetClientFromRegion(EGameLiftRegion Region)
{
	Aws::String RegionString = TCHAR_TO_UTF8(GetRegionString(Region));
	auto RegionClient = GameLiftClientMap.find(RegionString);
	if (RegionClient != GameLiftClientMap.end())
	{
		return RegionClient->second;
	}

	return NULL;
}

void FGameLiftClientObject::UpdateGameSession(FString SessionId, FString SessionName)
{
	Aws::GameLift::Model::UpdateGameSessionRequest Request;
	Request.SetGameSessionId(TCHAR_TO_UTF8(*SessionId));
	Request.SetName(TCHAR_TO_UTF8(*SessionName));
	GameLiftClient->UpdateGameSession(Request);
}


void FGameLiftClientObject::Internal_InitGameLiftClientSDK(const FString& AccessKey, const FString& Secret, EGameLiftRegion Region, bool bUsingGameLiftLocal, int32 LocalPort)
{
#if WITH_GAMELIFTCLIENTSDK
	Aws::Client::ClientConfiguration ClientConfig;
	Aws::Auth::AWSCredentials Credentials;

	ClientConfig.connectTimeoutMs = 10000;
	ClientConfig.requestTimeoutMs = 10000;
	//ClientConfig.region = TCHAR_TO_UTF8(*Region);
	
	bIsUsingGameLiftLocal = bUsingGameLiftLocal;

	// GameLiftLocal
	if (bUsingGameLiftLocal)
	{
		ClientConfig.scheme = Aws::Http::Scheme::HTTP;
		const FString HostAddress = FString::Printf(TEXT("localhost:%i"), LocalPort);
		ClientConfig.endpointOverride = TCHAR_TO_UTF8(*HostAddress);
		LOG_WARNING("GameLift is currently configured to use GameLift Local.");
	}

	Credentials = Aws::Auth::AWSCredentials(TCHAR_TO_UTF8(*AccessKey), TCHAR_TO_UTF8(*Secret));
	//GameLiftClient = new Aws::GameLift::GameLiftClient(Credentials, ClientConfig);

	CurrentRegion = Region;

	ClientConfig.region = TCHAR_TO_UTF8(GetRegionString(EGameLiftRegion::EGameLiftRegion_APJ));
	Aws::GameLift::GameLiftClient* GameLiftClientAPJ = new Aws::GameLift::GameLiftClient(Credentials, ClientConfig);
	GameLiftClientMap.emplace(ClientConfig.region, GameLiftClientAPJ);

	ClientConfig.region = TCHAR_TO_UTF8(GetRegionString(EGameLiftRegion::EGameLiftRegion_NA));
	Aws::GameLift::GameLiftClient* GameLiftClientNA = new Aws::GameLift::GameLiftClient(Credentials, ClientConfig);
	GameLiftClientMap.emplace(ClientConfig.region, GameLiftClientNA);

	GameLiftClient = GameLiftClientMap.find(TCHAR_TO_UTF8(GetRegionString(Region)))->second;

#endif
}

TSharedPtr<FGameLiftClientObject, ESPMode::ThreadSafe> FGameLiftClientObject::CreateGameLiftObject(const FString& AccessKey, const FString& Secret, EGameLiftRegion Region /*= "us-east-1"*/, bool bUsingGameLiftLocal /*= false*/, int32 LocalPort /*= 9080*/)
{
#if WITH_GAMELIFTCLIENTSDK
	TSharedPtr<FGameLiftClientObject, ESPMode::ThreadSafe> Proxy = MakeShareable(new FGameLiftClientObject());
	Proxy->Internal_InitGameLiftClientSDK(AccessKey, Secret, Region, bUsingGameLiftLocal, LocalPort);
	return Proxy;
#endif
	return nullptr;
}

TSharedPtr<FGameLiftPlaceGameSessionInQueue, ESPMode::ThreadSafe> FGameLiftClientObject::PlaceGameSessionInQueue(FGameLiftGameSessionConfig GameSessionProperties, const FString& QueueName, const FString& UniqueId, const TArray<TSharedRef<const FUniqueNetId>>& PlayerIds)
{
#if WITH_GAMELIFTCLIENTSDK
	TSharedPtr<FGameLiftPlaceGameSessionInQueue, ESPMode::ThreadSafe> Proxy = FGameLiftPlaceGameSessionInQueue::PlaceGameSessionInQueue(GameSessionProperties, QueueName, UniqueId, PlayerIds);
	Proxy->GameLiftClient = GameLiftClient;
	return Proxy;
#endif
	return nullptr;
}

TSharedPtr<FDescribeGameLiftGameSessionPlacement, ESPMode::ThreadSafe> FGameLiftClientObject::DescribeGameSessionPlacement(const FString& UniqueId)
{
#if WITH_GAMELIFTCLIENTSDK
	TSharedPtr<FDescribeGameLiftGameSessionPlacement, ESPMode::ThreadSafe> Proxy = FDescribeGameLiftGameSessionPlacement::DescribeGameSessionPlacement(UniqueId);
	Proxy->GameLiftClient = GameLiftClient;
	return Proxy;
#endif
	return nullptr;
}

TSharedPtr<FStartGameLiftMatchmaking, ESPMode::ThreadSafe> FGameLiftClientObject::StartMatchmaking(const FString& ConfigName, const TArray<TSharedRef<const FUniqueNetId>>& PlayerIds, const FString& MatchmakingTicket)
{
#if WITH_GAMELIFTCLIENTSDK
	TSharedPtr<FStartGameLiftMatchmaking, ESPMode::ThreadSafe> Proxy = FStartGameLiftMatchmaking::StartMatchmaking(ConfigName, PlayerIds, MatchmakingTicket);
	Proxy->GameLiftClient = GetClientFromRegion(EGameLiftRegion::EGameLiftRegion_NA);
	Proxy->RegionLatency = GetRegionLatencyAWS(GetCurrentRegion());

	return Proxy;
#endif
	return nullptr;
}

TSharedPtr<class FStopGameLiftMatchmaking, ESPMode::ThreadSafe> FGameLiftClientObject::StopMatchmaking(const FString& MatchmakingTicket)
{
#if WITH_GAMELIFTCLIENTSDK
	TSharedPtr<FStopGameLiftMatchmaking, ESPMode::ThreadSafe> Proxy = FStopGameLiftMatchmaking::StopMatchmaking(MatchmakingTicket);
	Proxy->GameLiftClient = GetClientFromRegion(EGameLiftRegion::EGameLiftRegion_NA);
	return Proxy;
#endif
	return nullptr;
}

TSharedPtr<class FDescribeGameLiftMatchmaking, ESPMode::ThreadSafe> FGameLiftClientObject::DescribeMatchmaking(const FString& TickId)
{
#if WITH_GAMELIFTCLIENTSDK
	TSharedPtr<FDescribeGameLiftMatchmaking, ESPMode::ThreadSafe> Proxy = FDescribeGameLiftMatchmaking::DescribeMatchmaking(TickId);
	Proxy->GameLiftClient = GetClientFromRegion(EGameLiftRegion::EGameLiftRegion_NA);
	return Proxy;
#endif
	return nullptr;
}

TSharedPtr<FGameLiftSearchGameSessions, ESPMode::ThreadSafe> FGameLiftClientObject::SearchGameLiftSessions(FGameLiftGameSessionSearchSettings GameSessionSearchSettings)
{
#if WITH_GAMELIFTCLIENTSDK
	TSharedPtr<FGameLiftSearchGameSessions, ESPMode::ThreadSafe> Proxy = FGameLiftSearchGameSessions::SearchGameSessions(GameSessionSearchSettings, bIsUsingGameLiftLocal);
	Proxy->GameLiftClient = GameLiftClient;
	Proxy->GameLiftClientMap = GameLiftClientMap;
	return Proxy;

#endif
	return nullptr;
}

TSharedPtr<FGameLiftDescribeGameSession, ESPMode::ThreadSafe> FGameLiftClientObject::DescribeGameSession(FString GameSessionID)
{
#if WITH_GAMELIFTCLIENTSDK
	TSharedPtr<FGameLiftDescribeGameSession, ESPMode::ThreadSafe> Proxy = FGameLiftDescribeGameSession::DescribeGameSession(GameSessionID);
	Proxy->GameLiftClient = GameLiftClient;
	return Proxy;
#endif
	return nullptr;
}

TSharedPtr<FGameLiftCreatePlayerSession, ESPMode::ThreadSafe> FGameLiftClientObject::CreatePlayerSession(FString GameSessionID, FString UniquePlayerID, EGameLiftRegion GameLiftRegion)
{
#if WITH_GAMELIFTCLIENTSDK
	TSharedPtr<FGameLiftCreatePlayerSession, ESPMode::ThreadSafe> Proxy = FGameLiftCreatePlayerSession::CreatePlayerSession(GameSessionID, UniquePlayerID, GameLiftRegion);
	Proxy->GameLiftClient = GetClientFromRegion(GameLiftRegion);
	return Proxy;
#endif
	return nullptr;
}
