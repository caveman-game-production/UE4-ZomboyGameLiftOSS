// Created by YetiTech Studios.

#include "GameLiftClientApi.h"
#include "GameLiftClientGlobals.h"
#include "Async.h"
#include <string>
#include <codecvt>

#if WITH_GAMELIFTCLIENTSDK

#include "aws/gamelift/model/DescribeGameSessionDetailsRequest.h"
#include "aws/gamelift/GameLiftClient.h"
#include "aws/core/utils/Outcome.h"
#include "aws/core/auth/AWSCredentialsProvider.h"
#include "aws/gamelift/model/GameProperty.h"
#include "aws/gamelift/model/SearchGameSessionsRequest.h"
#include "aws/gamelift/model/DescribeGameSessionsRequest.h"
#include "aws/gamelift/model/StartGameSessionPlacementRequest.h"
#include "aws/gamelift/model/DescribeGameSessionPlacementRequest.h"
#include "aws/gamelift/model/StartMatchmakingRequest.h"
#include "aws/gamelift/model/DescribeMatchmakingRequest.h"
#include "aws/gamelift/model/StopMatchmakingRequest.h"
#include "aws/gamelift/model/DescribeGameSessionQueuesRequest.h"


#include "aws/gamelift/model/CreatePlayerSessionRequest.h"
#include "aws/gamelift/model/CreateGameSessionRequest.h"
#include <aws/core/http/HttpRequest.h>
#endif

TSharedPtr<FGameLiftPlaceGameSessionInQueue, ESPMode::ThreadSafe> FGameLiftPlaceGameSessionInQueue::PlaceGameSessionInQueue(FGameLiftGameSessionConfig GameSessionProperties, const FString& QueueName, const FString& UniqueId, const TArray<TSharedRef<const FUniqueNetId>>& PlayerIds)
{
#if WITH_GAMELIFTCLIENTSDK
	TSharedPtr<FGameLiftPlaceGameSessionInQueue, ESPMode::ThreadSafe> Proxy = MakeShareable(new FGameLiftPlaceGameSessionInQueue());
	Proxy->SessionConfig = GameSessionProperties;
	Proxy->QueueName = QueueName;
	Proxy->UniqueId = UniqueId;
	Proxy->PlayerIds = PlayerIds;
	return Proxy;
#endif
	return nullptr;
}

EActivateStatus FGameLiftPlaceGameSessionInQueue::Activate()
{
#if WITH_GAMELIFTCLIENTSDK
	LOG_NORMAL("Preparing to place game session...");

	if (OnPlaceGameSessionSuccessDelegate.IsBound() == false)
	{
		LOG_ERROR("No functions were bound to OnCreateGameSessionSuccess multi-cast delegate! Aborting Activate.");
		return EActivateStatus::ACTIVATE_NoSuccessCallback;
	}

	if (OnPlaceGameSessionFailedDelegate.IsBound() == false)
	{
		LOG_ERROR("No functions were bound to OnCreateGameSessionFailed multi-cast delegate! Aborting Activate.");
		return EActivateStatus::ACTIVATE_NoFailCallback;
	}

	Aws::GameLift::Model::StartGameSessionPlacementRequest GameSessionPlacementRequest;

	GameSessionPlacementRequest.SetGameSessionQueueName(TCHAR_TO_UTF8(*QueueName));
	GameSessionPlacementRequest.SetPlacementId(TCHAR_TO_UTF8(*UniqueId));

	GameSessionPlacementRequest.SetMaximumPlayerSessionCount(SessionConfig.GetMaxPlayers());

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

	for (FGameLiftGameSessionServerProperties ServerDynamicSetting : SessionConfig.GetGameSessionDynamicProperties())
	{
		LOG_NORMAL("********************************************************************");
		if (!ServerDynamicSetting.Key.IsEmpty() && !ServerDynamicSetting.Value.IsEmpty())
		{
			JsonObject->SetStringField(ServerDynamicSetting.Key, ServerDynamicSetting.Value);
		}

		LOG_NORMAL(FString::Printf(TEXT("New Dynamic GameProperty added. Key: (%s) Value: (%s)"), *ServerDynamicSetting.Key, *ServerDynamicSetting.Value));
	}

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	GameSessionPlacementRequest.SetGameSessionName(TCHAR_TO_UTF8(*OutputString));

	for (FGameLiftGameSessionServerProperties ServerStaticSetting : SessionConfig.GetGameSessionStaticProperties())
	{
		LOG_NORMAL("********************************************************************");
		if (!ServerStaticSetting.Key.IsEmpty() && !ServerStaticSetting.Value.IsEmpty())
		{
			Aws::GameLift::Model::GameProperty MapProperty;
			MapProperty.SetKey(TCHAR_TO_UTF8(*ServerStaticSetting.Key));
			MapProperty.SetValue(TCHAR_TO_UTF8(*ServerStaticSetting.Value));
			GameSessionPlacementRequest.AddGameProperties(MapProperty);
		}
		
		LOG_NORMAL(FString::Printf(TEXT("New Static GameProperty added. Key: (%s) Value: (%s)"), *ServerStaticSetting.Key, *ServerStaticSetting.Value));
	}


	Aws::Vector<Aws::GameLift::Model::DesiredPlayerSession> DesiredPlayerSessions;
	Aws::Vector<Aws::GameLift::Model::PlayerLatency> Latencies;

	for (TSharedRef<const FUniqueNetId> PlayerId : PlayerIds)
	{
		const char* PlayerIdStr = TCHAR_TO_UTF8(*PlayerId->ToString());

		Aws::GameLift::Model::DesiredPlayerSession DesiredPlayerSession;
		DesiredPlayerSession.SetPlayerId(PlayerIdStr);
		DesiredPlayerSessions.push_back(DesiredPlayerSession);

		//Aws::GameLift::Model::PlayerLatency PlayerLatency;
		//PlayerLatency.SetPlayerId
	}

	GameSessionPlacementRequest.SetDesiredPlayerSessions(DesiredPlayerSessions);
	GameSessionPlacementRequest.SetPlayerLatencies(Latencies);
	
	Aws::GameLift::StartGameSessionPlacementResponseReceivedHandler Handler;
	Handler = std::bind(&FGameLiftPlaceGameSessionInQueue::OnPlacedGameSessionComplete, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

	GameLiftClient->StartGameSessionPlacementAsync(GameSessionPlacementRequest, Handler);

	return EActivateStatus::ACTIVATE_Success;

#endif

	return EActivateStatus::ACTIVATE_NoGameLift;
}

void FGameLiftPlaceGameSessionInQueue::OnPlacedGameSessionComplete(const Aws::GameLift::GameLiftClient* Client, const Aws::GameLift::Model::StartGameSessionPlacementRequest& Request, const Aws::GameLift::Model::StartGameSessionPlacementOutcome& Outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& Context)
{
	if (Outcome.IsSuccess())
	{
		LOG_NORMAL("game session has been placed successfully...");

		Aws::GameLift::Model::GameSessionPlacementState Status = Outcome.GetResult().GetGameSessionPlacement().GetStatus();

		if (Status == Aws::GameLift::Model::GameSessionPlacementState::PENDING || Status == Aws::GameLift::Model::GameSessionPlacementState::FULFILLED)
		{
			OnPlaceGameSessionSuccessDelegate.ExecuteIfBound(true);
		}
		else
		{
			OnPlaceGameSessionFailedDelegate.ExecuteIfBound(TEXT("Error: GameSessionPlacement is in wrong state."));
		}
	}
	else
	{
		const FString MyErrorMessage = FString(Outcome.GetError().GetMessage().c_str());
		LOG_ERROR("Received FGameLiftPlaceGameSessionInQueue with failed outcome. Error: " + MyErrorMessage);
		OnPlaceGameSessionFailedDelegate.ExecuteIfBound(MyErrorMessage);
	}
}

TSharedPtr<FDescribeGameLiftGameSessionPlacement, ESPMode::ThreadSafe> FDescribeGameLiftGameSessionPlacement::DescribeGameSessionPlacement(const FString& UniqueId)
{
#if WITH_GAMELIFTCLIENTSDK
	TSharedPtr<FDescribeGameLiftGameSessionPlacement, ESPMode::ThreadSafe> Proxy = MakeShareable(new FDescribeGameLiftGameSessionPlacement());
	Proxy->UniqueId = UniqueId;
	return Proxy;
#endif
	return nullptr;
}

EActivateStatus FDescribeGameLiftGameSessionPlacement::Activate()
{
#if WITH_GAMELIFTCLIENTSDK

	if (OnDescribeGameLiftGameSessionPlacementSuccessDelegate.IsBound() == false)
	{
		LOG_ERROR("No functions were bound to OnCreateGameSessionSuccess multi-cast delegate! Aborting Activate.");
		return EActivateStatus::ACTIVATE_NoSuccessCallback;
	}

	if (OnDescribeGameLiftGameSessionPlacementFailedDelegate.IsBound() == false)
	{
		LOG_ERROR("No functions were bound to OnCreateGameSessionFailed multi-cast delegate! Aborting Activate.");
		return EActivateStatus::ACTIVATE_NoFailCallback;
	}

	Aws::GameLift::Model::DescribeGameSessionPlacementRequest DescribeSessionPlacementRequest;
	DescribeSessionPlacementRequest.SetPlacementId(TCHAR_TO_UTF8(*UniqueId));

	Aws::GameLift::DescribeGameSessionPlacementResponseReceivedHandler Handler;
	Handler = std::bind(&FDescribeGameLiftGameSessionPlacement::OnDescribeSessionPlacement, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

	GameLiftClient->DescribeGameSessionPlacementAsync(DescribeSessionPlacementRequest, Handler);

	return EActivateStatus::ACTIVATE_Success;

#endif

	return EActivateStatus::ACTIVATE_NoGameLift;
}

void FDescribeGameLiftGameSessionPlacement::OnDescribeSessionPlacement(const Aws::GameLift::GameLiftClient* Client, const Aws::GameLift::Model::DescribeGameSessionPlacementRequest& Request, const Aws::GameLift::Model::DescribeGameSessionPlacementOutcome& Outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& Context)
{
	if (Outcome.IsSuccess())
	{
		Aws::GameLift::Model::GameSessionPlacementState Status = Outcome.GetResult().GetGameSessionPlacement().GetStatus();
		if (Status == Aws::GameLift::Model::GameSessionPlacementState::PENDING || Status == Aws::GameLift::Model::GameSessionPlacementState::FULFILLED)
		{
			const FString GameSessionID = FString(Outcome.GetResult().GetGameSessionPlacement().GetGameSessionId().c_str());
			const FString IPAddress = FString(Outcome.GetResult().GetGameSessionPlacement().GetIpAddress().c_str());
			const FString Port = FString::FromInt(Outcome.GetResult().GetGameSessionPlacement().GetPort());

			const Aws::Vector<Aws::GameLift::Model::PlacedPlayerSession>& PlacedPlayerSessions = Outcome.GetResult().GetGameSessionPlacement().GetPlacedPlayerSessions();
			OnDescribeGameLiftGameSessionPlacementSuccessDelegate.ExecuteIfBound(Status, GameSessionID, IPAddress, Port, PlacedPlayerSessions);
		}
		else
		{
			OnDescribeGameLiftGameSessionPlacementFailedDelegate.ExecuteIfBound(TEXT("Error: OnDescribeSessionPlacement is in wrong state."));
		}
	}
	else
	{
		const FString MyErrorMessage = FString(Outcome.GetError().GetMessage().c_str());
		LOG_ERROR("Received FGameLiftPlaceGameSessionInQueue with failed outcome. Error: " + MyErrorMessage);
		OnDescribeGameLiftGameSessionPlacementFailedDelegate.ExecuteIfBound(MyErrorMessage);
	}
}

TSharedPtr<FStartGameLiftMatchmaking, ESPMode::ThreadSafe> FStartGameLiftMatchmaking::StartMatchmaking(const FString& ConfigName, const TArray<TSharedRef<const FUniqueNetId>>& PlayerIds, FString MatchmakingTicketId)
{
#if WITH_GAMELIFTCLIENTSDK
	TSharedPtr<FStartGameLiftMatchmaking, ESPMode::ThreadSafe> Proxy = MakeShareable(new FStartGameLiftMatchmaking());
	Proxy->ConfigName = ConfigName;
	Proxy->PlayerIds = PlayerIds;
	Proxy->MatchmakingTicketId = MatchmakingTicketId;
	return Proxy;
#endif
	return nullptr;
}

void FStartGameLiftMatchmaking::ChangeMatchmakingTicket(const FString& NewMatchmakingTicketId)
{
	MatchmakingTicketId = NewMatchmakingTicketId;
}

EActivateStatus FStartGameLiftMatchmaking::Activate()
{
#if WITH_GAMELIFTCLIENTSDK

	if (OnStartMatchmakingSuccessDelegate.IsBound() == false)
	{
		LOG_ERROR("No functions were bound to OnCreateGameSessionSuccess multi-cast delegate! Aborting Activate.");
		return EActivateStatus::ACTIVATE_NoSuccessCallback;
	}

	if (OnStartMatchmakingFailedDelegate.IsBound() == false)
	{
		LOG_ERROR("No functions were bound to OnCreateGameSessionFailed multi-cast delegate! Aborting Activate.");
		return EActivateStatus::ACTIVATE_NoFailCallback;
	}

	Aws::Vector<Aws::GameLift::Model::Player> MatchmakingPlayers;
	for (TSharedRef<const FUniqueNetId> PlayerId : PlayerIds)
	{
		if (PlayerId->IsValid())
		{
			Aws::GameLift::Model::Player AwsPlayer;
			AwsPlayer.SetPlayerId(TCHAR_TO_UTF8(*PlayerId->ToString()));
			AwsPlayer.SetLatencyInMs(RegionLatency);
			MatchmakingPlayers.push_back(AwsPlayer);
		}		
	}

	if (MatchmakingPlayers.size() > 0)
	{
		Aws::GameLift::Model::StartMatchmakingRequest StartMatchmakingRequest;
		StartMatchmakingRequest.SetTicketId(TCHAR_TO_UTF8(*MatchmakingTicketId));
		StartMatchmakingRequest.SetConfigurationName(TCHAR_TO_UTF8(*ConfigName));
		StartMatchmakingRequest.SetPlayers(MatchmakingPlayers);

		Aws::GameLift::StartMatchmakingResponseReceivedHandler Handler;
		Handler = std::bind(&FStartGameLiftMatchmaking::OnGameLiftMatchmakingComplete, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

		GameLiftClient->StartMatchmakingAsync(StartMatchmakingRequest, Handler);
		return EActivateStatus::ACTIVATE_Success;
	}

#endif

	return EActivateStatus::ACTIVATE_NoGameLift;
}

void FStartGameLiftMatchmaking::OnGameLiftMatchmakingComplete(const Aws::GameLift::GameLiftClient* Client, const Aws::GameLift::Model::StartMatchmakingRequest& Request, const Aws::GameLift::Model::StartMatchmakingOutcome& Outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& Context)
{
	if (Outcome.IsSuccess())
	{
		Aws::GameLift::Model::MatchmakingConfigurationStatus Status = Outcome.GetResult().GetMatchmakingTicket().GetStatus();
		if (Status != Aws::GameLift::Model::MatchmakingConfigurationStatus::FAILED
			&& Status != Aws::GameLift::Model::MatchmakingConfigurationStatus::CANCELLED
			&& Status != Aws::GameLift::Model::MatchmakingConfigurationStatus::TIMED_OUT)
		{
			FString TickId = FString(Outcome.GetResult().GetMatchmakingTicket().GetTicketId().c_str());
			if (!TickId.IsEmpty())
			{
				OnStartMatchmakingSuccessDelegate.ExecuteIfBound(Status, TickId);
			}
			else
			{
				OnStartMatchmakingFailedDelegate.ExecuteIfBound(TEXT("Error: OnGameLiftMatchmakingComplete tick is empty."));
			}
		}
		else
		{
			OnStartMatchmakingFailedDelegate.ExecuteIfBound(TEXT("Error: OnGameLiftMatchmakingComplete is in wrong state."));
		}
	}
	else
	{
		const FString MyErrorMessage = FString(Outcome.GetError().GetMessage().c_str());
		LOG_ERROR("Received FGameLiftPlaceGameSessionInQueue with failed outcome. Error: " + MyErrorMessage);
		OnStartMatchmakingFailedDelegate.ExecuteIfBound(MyErrorMessage);
	}
}

TSharedPtr<FDescribeGameLiftMatchmaking, ESPMode::ThreadSafe> FDescribeGameLiftMatchmaking::DescribeMatchmaking(const FString& TickId)
{
#if WITH_GAMELIFTCLIENTSDK
	TSharedPtr<FDescribeGameLiftMatchmaking, ESPMode::ThreadSafe> Proxy = MakeShareable(new FDescribeGameLiftMatchmaking());
	Proxy->TickId = TickId;
	return Proxy;
#endif
	return nullptr;
}

EActivateStatus FDescribeGameLiftMatchmaking::Activate()
{
#if WITH_GAMELIFTCLIENTSDK

	if (OnDescribeMatchmakingSuccessDelegate.IsBound() == false)
	{
		LOG_ERROR("No functions were bound to OnCreateGameSessionSuccess multi-cast delegate! Aborting Activate.");
		return EActivateStatus::ACTIVATE_NoSuccessCallback;
	}

	if (OnDescribeMatchmakingFailedDelegate.IsBound() == false)
	{
		LOG_ERROR("No functions were bound to OnCreateGameSessionFailed multi-cast delegate! Aborting Activate.");
		return EActivateStatus::ACTIVATE_NoFailCallback;
	}

	Aws::GameLift::Model::DescribeMatchmakingRequest Request;
	Aws::Vector<Aws::String> TicketIds;
	TicketIds.push_back(TCHAR_TO_UTF8(*TickId));
	Request.SetTicketIds(TicketIds);

	Aws::GameLift::DescribeMatchmakingResponseReceivedHandler Handler;
	Handler = std::bind(&FDescribeGameLiftMatchmaking::OnDescribeMatchmakingComplete, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

	GameLiftClient->DescribeMatchmakingAsync(Request, Handler);

	return EActivateStatus::ACTIVATE_Success;
#endif

	return EActivateStatus::ACTIVATE_NoGameLift;
}

void FDescribeGameLiftMatchmaking::OnDescribeMatchmakingComplete(const  Aws::GameLift::GameLiftClient* Client, const  Aws::GameLift::Model::DescribeMatchmakingRequest& Request, const  Aws::GameLift::Model::DescribeMatchmakingOutcome& Outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& Context)
{
	if (Outcome.IsSuccess())
	{
		if (Outcome.GetResult().GetTicketList().size() > 0)
		{
			Aws::GameLift::Model::MatchmakingTicket TicketResult = Outcome.GetResult().GetTicketList()[0];
			Aws::GameLift::Model::MatchmakingConfigurationStatus Status = TicketResult.GetStatus();

			if (Status != Aws::GameLift::Model::MatchmakingConfigurationStatus::FAILED
				&& Status != Aws::GameLift::Model::MatchmakingConfigurationStatus::CANCELLED
				&& Status != Aws::GameLift::Model::MatchmakingConfigurationStatus::TIMED_OUT)
			{
				FString TicketId = FString(TicketResult.GetTicketId().c_str());
				if (!TicketId.IsEmpty())
				{
					FString GameSessionId = FString(TicketResult.GetGameSessionConnectionInfo().GetGameSessionArn().c_str());
					FString IPAddress = FString(TicketResult.GetGameSessionConnectionInfo().GetIpAddress().c_str());
					FString Port = FString::FromInt(TicketResult.GetGameSessionConnectionInfo().GetPort());

					auto GameLiftPlayerSessions = TicketResult.GetGameSessionConnectionInfo().GetMatchedPlayerSessions();
					auto GameLiftPlayers = TicketResult.GetPlayers();
					
					OnDescribeMatchmakingSuccessDelegate.ExecuteIfBound(Status, GameSessionId, IPAddress, Port, GameLiftPlayerSessions, GameLiftPlayers);
				}
				else
				{
					OnDescribeMatchmakingFailedDelegate.ExecuteIfBound(TEXT("Error: OnDescribeMatchmakingComplete tick is empty."), Status, false);
				}
			}
			else
			{
				OnDescribeMatchmakingFailedDelegate.ExecuteIfBound(TEXT("Error: OnDescribeMatchmakingComplete is in wrong state."), Status, false);

			}
		}
		else
		{
			OnDescribeMatchmakingFailedDelegate.ExecuteIfBound(TEXT("Error: OnDescribeMatchmakingComplete no ticket has been returned."), Aws::GameLift::Model::MatchmakingConfigurationStatus::FAILED, false);
		}
	}
	else
	{
		const FString MyErrorMessage = FString(Outcome.GetError().GetMessage().c_str());
		LOG_ERROR("Received FGameLiftPlaceGameSessionInQueue with failed outcome. Error: " + MyErrorMessage);
		OnDescribeMatchmakingFailedDelegate.ExecuteIfBound(MyErrorMessage, Aws::GameLift::Model::MatchmakingConfigurationStatus::FAILED, true);
	}
}

TSharedPtr<FStopGameLiftMatchmaking, ESPMode::ThreadSafe> FStopGameLiftMatchmaking::StopMatchmaking(const FString& TickId)
{
#if WITH_GAMELIFTCLIENTSDK
	TSharedPtr<FStopGameLiftMatchmaking, ESPMode::ThreadSafe> Proxy = MakeShareable(new FStopGameLiftMatchmaking());
	Proxy->TickId = TickId;
	return Proxy;
#endif
	return nullptr;
}

EActivateStatus FStopGameLiftMatchmaking::Activate()
{
#if WITH_GAMELIFTCLIENTSDK

	if (GameLiftClient)
	{
		if (OnStopGameLiftMatchmakingSuccessDelegate.IsBound() == false)
		{
			LOG_ERROR("No functions were bound to OnCreateGameSessionSuccess multi-cast delegate! Aborting Activate.");
			return EActivateStatus::ACTIVATE_NoSuccessCallback;
		}

		if (OnStopGameLiftMatchmakingFailedDelegate.IsBound() == false)
		{
			LOG_ERROR("No functions were bound to OnCreateGameSessionFailed multi-cast delegate! Aborting Activate.");
			return EActivateStatus::ACTIVATE_NoFailCallback;
		}

		Aws::GameLift::Model::StopMatchmakingRequest Request;
		Request.SetTicketId(TCHAR_TO_UTF8(*TickId));

		Aws::GameLift::StopMatchmakingResponseReceivedHandler Handler;
		Handler = std::bind(&FStopGameLiftMatchmaking::OnStopMatchmakingComplete, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

		GameLiftClient->StopMatchmakingAsync(Request, Handler);

		return EActivateStatus::ACTIVATE_Success;
	}

#endif
	return EActivateStatus::ACTIVATE_NoGameLift;
}

void FStopGameLiftMatchmaking::OnStopMatchmakingComplete(const  Aws::GameLift::GameLiftClient* Client, const  Aws::GameLift::Model::StopMatchmakingRequest& Request, const  Aws::GameLift::Model::StopMatchmakingOutcome& Outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& Context)
{
	if (Outcome.IsSuccess())
	{
		OnStopGameLiftMatchmakingSuccessDelegate.ExecuteIfBound();
	}
	else
	{
		OnStopGameLiftMatchmakingFailedDelegate.ExecuteIfBound();
	}
}

TSharedPtr<FGameLiftSearchGameSessions, ESPMode::ThreadSafe> FGameLiftSearchGameSessions::SearchGameSessions(FGameLiftGameSessionSearchSettings SearchSettings, bool bIsGameLiftLocal)
{
#if WITH_GAMELIFTCLIENTSDK
	TSharedPtr<FGameLiftSearchGameSessions, ESPMode::ThreadSafe> Proxy = MakeShareable(new FGameLiftSearchGameSessions());
	Proxy->SearchSettings = SearchSettings;
	Proxy->bIsUsingGameLiftLocal = bIsGameLiftLocal;
	return Proxy;
#endif
	return nullptr;
}

EActivateStatus FGameLiftSearchGameSessions::Activate()
{
#if WITH_GAMELIFTCLIENTSDK
	if (GameLiftClient)
	{
		if (OnSearchGameSessionsSuccessDelegate.IsBound() == false)
		{
			LOG_ERROR("No functions were bound to OnSearchGameSessionsSuccess multi-cast delegate! Aborting Activate.");
			return EActivateStatus::ACTIVATE_NoSuccessCallback;
		}

		if (OnSearchGameSessionsFailedDelegate.IsBound() == false)
		{
			LOG_ERROR("No functions were bound to OnSearchGameSessionsFailed multi-cast delegate! Aborting Activate.");
			return EActivateStatus::ACTIVATE_NoFailCallback;
		}

		Aws::GameLift::Model::DescribeGameSessionQueuesRequest DescribeQueueRequest;
		Aws::String SessionQueueNameStr = TCHAR_TO_UTF8(*SearchSettings.GetSessionQueueName());
		Aws::Vector<Aws::String> QueueNames;
		QueueNames.push_back(SessionQueueNameStr);
		DescribeQueueRequest.SetNames(QueueNames);

		bool bHaveSuccessfulResult = false;
		TArray<FGameLiftGameSessionSearchResult> SearchResults;


		Aws::GameLift::Model::DescribeGameSessionQueuesOutcome DescribeGameSessionOutcome = GameLiftClient->DescribeGameSessionQueues(DescribeQueueRequest);
		if (DescribeGameSessionOutcome.IsSuccess() && DescribeGameSessionOutcome.GetResult().GetGameSessionQueues().size() > 0)
		{
			Aws::GameLift::Model::GameSessionQueue GameSessionQueue = DescribeGameSessionOutcome.GetResult().GetGameSessionQueues()[0];

			Aws::Vector<Aws::GameLift::Model::GameSessionQueueDestination> Destinations = GameSessionQueue.GetDestinations();
			for (const Aws::GameLift::Model::GameSessionQueueDestination& Destination : Destinations)
			{
				FString DestinationArn = FString(Destination.GetDestinationArn().c_str());
				TArray<FString> DestinationParseArray;
				DestinationArn.ParseIntoArray(DestinationParseArray, TEXT("::alias/"), true);
				if (DestinationParseArray.Num() > 1)
				{
					FString Head = DestinationParseArray[0];
					FString Alias = DestinationParseArray[1];

					LOG_NORMAL("Head: " + Head);
					LOG_NORMAL("Alias ID: " + Alias);

					TArray<FString> RegionParseArray;
					Head.ParseIntoArray(RegionParseArray, TEXT("gamelift:"), true);
					if (RegionParseArray.Num() > 1)
					{
						Aws::GameLift::Model::DescribeGameSessionsRequest Request;
						Request.SetAliasId(TCHAR_TO_UTF8(*Alias));
						Request.SetStatusFilter("ACTIVE");

						FString RegionString = RegionParseArray[1];
						LOG_NORMAL("Region: " + RegionString);

						EGameLiftRegion GameLiftRegion = GetRegion(*RegionString);

						if (GameLiftRegion != EGameLiftRegion::EGameLiftRegion_None)
						{
							auto RegionGameLiftClientItr = GameLiftClientMap.find(TCHAR_TO_UTF8(*RegionString));
							if (RegionGameLiftClientItr != GameLiftClientMap.end())
							{
								Aws::GameLift::GameLiftClient* RegionGameLiftClient = RegionGameLiftClientItr->second;

								Aws::GameLift::Model::DescribeGameSessionsOutcome Outcome = RegionGameLiftClient->DescribeGameSessions(Request);
								if (Outcome.IsSuccess())
								{
									const Aws::Vector<Aws::GameLift::Model::GameSession>& AwsGameSessions = Outcome.GetResult().GetGameSessions();
									for (auto const &AwsGameSession : AwsGameSessions)
									{
										if (AwsGameSession.GetStatus() == Aws::GameLift::Model::GameSessionStatus::ACTIVE)
										{
											bHaveSuccessfulResult = true;
											
											LOG_NORMAL("Have valid result");

											FGameLiftGameSessionSearchResult SearchResult;
											SearchResult.SessionId = FString(AwsGameSession.GetGameSessionId().c_str());
											SearchResult.MatchmakerData = FString(AwsGameSession.GetMatchmakerData().c_str());
											SearchResult.SessionRegion = GameLiftRegion;
											SearchResult.CurrentPlayerCount = AwsGameSession.GetCurrentPlayerSessionCount();
											SearchResult.MaxPlayerCount = AwsGameSession.GetMaximumPlayerSessionCount();

											LOG_NORMAL("Result Matchmaker data: " + SearchResult.MatchmakerData);

											FString NameString = FString(UTF8_TO_TCHAR(AwsGameSession.GetName().c_str()));

											TSharedPtr<FJsonObject> SessionDataObject = MakeShareable(new FJsonObject());
											TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(NameString);
											
											if (FJsonSerializer::Deserialize(JsonReader, SessionDataObject) && SessionDataObject.IsValid())
											{
												for (auto& GameSessionProperty : SessionDataObject->Values)
												{
													FString Key = GameSessionProperty.Key;
													if (Key.IsEmpty()) continue;

													TSharedPtr<FJsonValue> JsonValue = GameSessionProperty.Value;
													FString Value;
													if (JsonValue.IsValid() && JsonValue->TryGetString(Value))
													{
														SearchResult.DynamicProperties.Add(Key, Value);
													}
												}
											}

											for (const auto& Property : AwsGameSession.GetGameProperties())
											{
												FString Key = FString(UTF8_TO_TCHAR(Property.GetKey().c_str()));
												FString Value = FString(UTF8_TO_TCHAR(Property.GetValue().c_str()));
												if (Key.IsEmpty() || Value.IsEmpty()) continue;

												SearchResult.StaticProperties.Add(Key, Value);
											}

											SearchResults.Add(SearchResult);
										}
									}
								}
							}
						}
					}
				}
			}
		}

		if (bHaveSuccessfulResult)
		{
			if (SearchResults.Num() > 0)
			{
				OnSearchGameSessionsSuccessDelegate.ExecuteIfBound(SearchResults);
			}
			else
			{
				OnSearchGameSessionsFailedDelegate.ExecuteIfBound(TEXT("No Result."));
			}
		}
		else
		{
			//const FString MyErrorMessage = FString(Outcome.GetError().GetMessage().c_str());
			OnSearchGameSessionsFailedDelegate.ExecuteIfBound(TEXT("Search for sessions failed"));
		}

		return EActivateStatus::ACTIVATE_Success;

	}
#endif
	return EActivateStatus::ACTIVATE_NoGameLift;
}

TSharedPtr<FGameLiftDescribeGameSession, ESPMode::ThreadSafe> FGameLiftDescribeGameSession::DescribeGameSession(FString GameSessionID)
{
#if WITH_GAMELIFTCLIENTSDK
	TSharedPtr<FGameLiftDescribeGameSession, ESPMode::ThreadSafe> Proxy =  MakeShareable(new FGameLiftDescribeGameSession());
	Proxy->SessionID = GameSessionID;
	return Proxy;
#endif
	return nullptr;
}

EActivateStatus FGameLiftDescribeGameSession::Activate()
{
#if WITH_GAMELIFTCLIENTSDK
	if (GameLiftClient)
	{
		LOG_NORMAL("Preparing to describe game session...");

		if (OnDescribeGameSessionStateSuccessDelegate.IsBound() == false)
		{
			LOG_ERROR("No functions were bound to OnDescribeGameSessionStateSuccess multi-cast delegate! Aborting Activate.");
			return EActivateStatus::ACTIVATE_NoSuccessCallback;
		}

		if (OnDescribeGameSessionStateFailedDelegate.IsBound() == false)
		{
			LOG_ERROR("No functions were bound to OnDescribeGameSessionStateFailed multi-cast delegate! Aborting Activate.");
			return EActivateStatus::ACTIVATE_NoFailCallback;
		}

		Aws::GameLift::Model::DescribeGameSessionDetailsRequest Request;
		Request.SetGameSessionId(TCHAR_TO_UTF8(*SessionID));

		Aws::GameLift::DescribeGameSessionDetailsResponseReceivedHandler Handler;
		Handler = std::bind(&FGameLiftDescribeGameSession::OnDescribeGameSessionState, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

		LOG_NORMAL("Requesting to describe game session with ID: " + SessionID);
		GameLiftClient->DescribeGameSessionDetailsAsync(Request, Handler);
		return EActivateStatus::ACTIVATE_Success;
	}
	LOG_ERROR("GameLiftClient is null. Did you call CreateGameLiftObject first?");
#endif
	return EActivateStatus::ACTIVATE_NoGameLift;
}

void FGameLiftDescribeGameSession::OnDescribeGameSessionState(const Aws::GameLift::GameLiftClient* Client, const Aws::GameLift::Model::DescribeGameSessionDetailsRequest& Request, const Aws::GameLift::Model::DescribeGameSessionDetailsOutcome& Outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& Context)
{
#if WITH_GAMELIFTCLIENTSDK
	if (Outcome.IsSuccess())
	{
		LOG_NORMAL("Received OnDescribeGameSessionState with Success outcome.");
		const FString MySessionID = FString(Outcome.GetResult().GetGameSessionDetails().data()->GetGameSession().GetGameSessionId().c_str());
		EGameLiftGameSessionStatus MySessionStatus = GetSessionState(Outcome.GetResult().GetGameSessionDetails().data()->GetGameSession().GetStatus());
		OnDescribeGameSessionStateSuccessDelegate.ExecuteIfBound(MySessionID, MySessionStatus);
	}
	else
	{
		const FString MyErrorMessage = FString(Outcome.GetError().GetMessage().c_str());
		LOG_ERROR("Received OnDescribeGameSessionState with failed outcome. Error: " + MyErrorMessage);
		OnDescribeGameSessionStateFailedDelegate.ExecuteIfBound(MyErrorMessage);
	}
#endif
}

EGameLiftGameSessionStatus FGameLiftDescribeGameSession::GetSessionState(const Aws::GameLift::Model::GameSessionStatus& Status)
{
#if WITH_GAMELIFTCLIENTSDK
	switch (Status)
	{
		case Aws::GameLift::Model::GameSessionStatus::ACTIVATING:
			return EGameLiftGameSessionStatus::STATUS_Activating;
		case Aws::GameLift::Model::GameSessionStatus::ACTIVE:
			return EGameLiftGameSessionStatus::STATUS_Active;
		case Aws::GameLift::Model::GameSessionStatus::ERROR_:
			return EGameLiftGameSessionStatus::STATUS_Error;
		case Aws::GameLift::Model::GameSessionStatus::NOT_SET:
			return EGameLiftGameSessionStatus::STATUS_NotSet;
		case Aws::GameLift::Model::GameSessionStatus::TERMINATED:
			return EGameLiftGameSessionStatus::STATUS_Terminated;
		case Aws::GameLift::Model::GameSessionStatus::TERMINATING:
			return EGameLiftGameSessionStatus::STATUS_Terminating;
		default:
			break;
	}
	checkNoEntry(); // This code block should never reach!
#endif
	return EGameLiftGameSessionStatus::STATUS_NoStatus; // Just a dummy return
}

TSharedPtr<FGameLiftCreatePlayerSession, ESPMode::ThreadSafe> FGameLiftCreatePlayerSession::CreatePlayerSession(FString GameSessionID, FString UniquePlayerID, EGameLiftRegion GameLiftRegion)
{
#if WITH_GAMELIFTCLIENTSDK
	TSharedPtr<FGameLiftCreatePlayerSession, ESPMode::ThreadSafe> Proxy = MakeShareable(new FGameLiftCreatePlayerSession());

	Proxy->GameSessionID = GameSessionID;
	Proxy->PlayerID = UniquePlayerID;
	Proxy->GameLiftRegion = GameLiftRegion;

	return Proxy;
#endif
	return nullptr;
}

EActivateStatus FGameLiftCreatePlayerSession::Activate()
{
#if WITH_GAMELIFTCLIENTSDK
	if (GameLiftClient)
	{		
		LOG_NORMAL("Preparing to create player session...");

		if (OnCreatePlayerSessionSuccessDelegate.IsBound() == false)
		{
			LOG_ERROR("No functions were bound to OnCreatePlayerSessionSuccess multi-cast delegate! Aborting Activate.");
			return EActivateStatus::ACTIVATE_NoSuccessCallback;
		}

		if (OnCreatePlayerSessionFailedDelegate.IsBound() == false)
		{
			LOG_ERROR("No functions were bound to OnCreatePlayerSessionFailed multi-cast delegate! Aborting Activate.");
			return EActivateStatus::ACTIVATE_NoFailCallback;
		}

		Aws::GameLift::Model::CreatePlayerSessionRequest Request;
		LOG_NORMAL("Setting game session ID: " + GameSessionID);
		Request.SetGameSessionId(TCHAR_TO_UTF8(*GameSessionID));
		LOG_NORMAL("Setting player ID: " + PlayerID);
		Request.SetPlayerId(TCHAR_TO_UTF8(*PlayerID));

		Aws::GameLift::CreatePlayerSessionResponseReceivedHandler Handler;
		Handler = std::bind(&FGameLiftCreatePlayerSession::OnCreatePlayerSession, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

		LOG_NORMAL("Creating new player session...");
		GameLiftClient->CreatePlayerSessionAsync(Request, Handler);
		return EActivateStatus::ACTIVATE_Success;
	}
	LOG_ERROR("GameLiftClient is null. Did you call CreateGameLiftObject first?");
#endif
	return EActivateStatus::ACTIVATE_NoGameLift;
}

void FGameLiftCreatePlayerSession::OnCreatePlayerSession(const Aws::GameLift::GameLiftClient* Client, const Aws::GameLift::Model::CreatePlayerSessionRequest& Request, const Aws::GameLift::Model::CreatePlayerSessionOutcome& Outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& Context)
{
#if WITH_GAMELIFTCLIENTSDK
	if (Outcome.IsSuccess())
	{
		LOG_NORMAL("Received OnCreatePlayerSession with Success outcome.");
		const FString ServerIpAddress = FString(Outcome.GetResult().GetPlayerSession().GetIpAddress().c_str());
		const FString ServerPort = FString::FromInt(Outcome.GetResult().GetPlayerSession().GetPort());
		const FString MyPlayerSessionID = FString(Outcome.GetResult().GetPlayerSession().GetPlayerSessionId().c_str());
		OnCreatePlayerSessionSuccessDelegate.ExecuteIfBound(ServerIpAddress, ServerPort, MyPlayerSessionID);
	}
	else
	{
		const FString MyErrorMessage = FString(Outcome.GetError().GetMessage().c_str());
		LOG_ERROR("Received OnCreatePlayerSession with failed outcome. Error: " + MyErrorMessage);
		OnCreatePlayerSessionFailedDelegate.ExecuteIfBound(MyErrorMessage);
	}
#endif
}
