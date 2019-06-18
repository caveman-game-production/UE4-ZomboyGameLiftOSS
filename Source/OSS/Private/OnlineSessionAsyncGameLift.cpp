#include "OnlineSessionAsyncGameLift.h"
#include "SocketSubsystem.h"
#include "OnlineSessionInterfaceZomboy.h"
#include "OnlineSubsystemZomboy.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionInterfaceZomboy.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineIdentityZomboy.h"
#include "OnlineSubsystemZomboyTypes.h"
#include <sstream>
#include <string>

#if WITH_GAMELIFTCLIENTSDK

/**
*	Get all relevant FOnlineSessionSettings data as a series of Key,Value pairs
*
* @param SessionSettings session settings to get key, value pairs from
* @param KeyValuePairs key value pair structure to add to
*/
static void GetKeyValuePairsFromSessionSettings(const FOnlineSessionSettings& SessionSettings, FZomboySessionKeyValuePairs& KeyValuePairs, EOnlineDataAdvertisementType::Type Type)
{
	for (FSessionSettings::TConstIterator It(SessionSettings.Settings); It; ++It)
	{
		const FOnlineSessionSetting& Setting = It.Value();
		if (Setting.AdvertisementType == Type)
		{
			FString KeyStr = It.Key().ToString();
			FString SettingStr = Setting.Data.ToString();
			if (!KeyStr.IsEmpty() && !SettingStr.IsEmpty())
			{
				KeyValuePairs.Add(KeyStr, SettingStr);
			}
			else
			{
				UE_LOG_ONLINE(Warning, TEXT("Empty session setting %s %s of type %s"), *KeyStr, *Setting.ToString(), EOnlineKeyValuePairDataType::ToString(Setting.Data.GetType()));
			}
		}
	}
}

static void GetGameLiftServerSettingsFromKeyValuePairs(const FOnlineSessionSettings& SessionSettings, TArray<FGameLiftGameSessionServerProperties>& GameLiftServerSettings, EOnlineDataAdvertisementType::Type Type)
{
	FZomboySessionKeyValuePairs DynamicSettingKeyValuePairs;
	GetKeyValuePairsFromSessionSettings(SessionSettings, DynamicSettingKeyValuePairs, Type);

	// Register session properties with Steam lobby
	for (FZomboySessionKeyValuePairs::TConstIterator It(DynamicSettingKeyValuePairs); It; ++It)
	{
		UE_LOG_ONLINE(Verbose, TEXT("Lobby Data (%s, %s, %s)"), *It.Key(), *It.Value(), *ToString(Type));

		FGameLiftGameSessionServerProperties ServerSetting;
		ServerSetting.Key = It.Key();
		ServerSetting.Value = It.Value();

		GameLiftServerSettings.Add(ServerSetting);
	}
}


FString FOnlineAsyncTaskCreateGameliftSession::ToString() const
{
	return FString::Printf(TEXT("FOnlineAsyncTaskCreateGameliftSession bWasSuccessful: %d"), bWasSuccessful);
}

void FOnlineAsyncTaskCreateGameliftSession::TaskThreadInit()
{
	if (Subsystem)
	{
		GameLiftObject = Subsystem->GetGameLiftClient();


		if (!GameLiftObject.IsValid())
		{
			FinishTaskThread(false);
			return;
		}

		FOnlineSessionZomboyPtr SessionInt = StaticCastSharedPtr<FOnlineSessionZomboy>(Subsystem->GetSessionInterface());

		if (FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName))
		{
			FOnlineIdentityZomboyPtr IdentityInt = StaticCastSharedPtr<FOnlineIdentityZomboy>(Subsystem->GetIdentityInterface());
			if (IdentityInt.IsValid())
			{
				if (IdentityInt->GetUniquePlayerId(0).IsValid() && IdentityInt->GetUniquePlayerId(0)->IsValid())
				{
					FString PlayerUniqueIdStr = IdentityInt->GetUniquePlayerId(0)->ToString();

					FGameLiftGameSessionConfig GameLiftSessonConfig;

					int MaxPlayers = Session->SessionSettings.NumPrivateConnections + Session->SessionSettings.NumPublicConnections;
					GameLiftSessonConfig.SetMaxPlayers(MaxPlayers);

					TArray<FGameLiftGameSessionServerProperties> DynamicServerSettings;
					GetGameLiftServerSettingsFromKeyValuePairs(Session->SessionSettings, DynamicServerSettings, EOnlineDataAdvertisementType::ViaOnlineService);
					GameLiftSessonConfig.SetGameSessionDynamicProperties(DynamicServerSettings);

					TArray<FGameLiftGameSessionServerProperties> StaticServerSettings;
					GetGameLiftServerSettingsFromKeyValuePairs(Session->SessionSettings, StaticServerSettings, EOnlineDataAdvertisementType::ViaPingOnly);
					GameLiftSessonConfig.SetGameSessionStaticProperties(StaticServerSettings);


					FGuid PlacementGUID = FGuid::NewGuid();
					SessionPlacementUniqueId = PlacementGUID.ToString();
					PlaceGameSessionObject = GameLiftObject->PlaceGameSessionInQueue(GameLiftSessonConfig, Subsystem->GetGameLiftQueueName(), SessionPlacementUniqueId, PlayerIds);

					if (PlaceGameSessionObject.IsValid())
					{
						PlaceGameSessionObject->OnPlaceGameSessionSuccessDelegate.BindRaw(this, &FOnlineAsyncTaskCreateGameliftSession::OnPlaceGameLiftGameSessionSuccess);
						PlaceGameSessionObject->OnPlaceGameSessionFailedDelegate.BindRaw(this, &FOnlineAsyncTaskCreateGameliftSession::OnPlaceGameLiftGameSessionFailed);

						EActivateStatus Status = PlaceGameSessionObject->Activate();
						if (Status != EActivateStatus::ACTIVATE_Success)
						{
							FinishTaskThread(false);
						}

						return;
					}
				}
			}
		}
	}

	FinishTaskThread(false);
}

void FOnlineAsyncTaskCreateGameliftSession::OnPlaceGameLiftGameSessionSuccess(bool bPlacementSuccessful)
{
	QuerySessionTimeStamp = FPlatformTime::Seconds();

	if (bPlacementSuccessful)
	{
		DescribeGameSessionPlacementObject = GameLiftObject->DescribeGameSessionPlacement(SessionPlacementUniqueId);
		DescribeGameSessionPlacementObject->OnDescribeGameLiftGameSessionPlacementSuccessDelegate.BindRaw(this, &FOnlineAsyncTaskCreateGameliftSession::OnDescribeGameSessionPlacementSuccess);
		DescribeGameSessionPlacementObject->OnDescribeGameLiftGameSessionPlacementFailedDelegate.BindRaw(this, &FOnlineAsyncTaskCreateGameliftSession::OnDescribeGameSessionPlacementFailed);
		if (DescribeGameSessionPlacementObject->Activate() != EActivateStatus::ACTIVATE_Success)
		{
			FinishTaskThread(false);
			return;
		}
	}
	else
	{
		FinishTaskThread(false);
	}
}

void FOnlineAsyncTaskCreateGameliftSession::OnPlaceGameLiftGameSessionFailed(const FString& ErroeMessage)
{
	FinishTaskThread(false);
}

void FOnlineAsyncTaskCreateGameliftSession::OnDescribeGameSessionPlacementSuccess(Aws::GameLift::Model::GameSessionPlacementState Status, const FString& SessionId, const FString& IPAddress, const FString& Port, const Aws::Vector<Aws::GameLift::Model::PlacedPlayerSession>& PlacedPlayerSessions)
{
	if (Status == Aws::GameLift::Model::GameSessionPlacementState::FULFILLED)
	{
		if (PlacedPlayerSessions.size() > 0)
		{
			FString PlayerSessionID = FString(PlacedPlayerSessions[0].GetPlayerSessionId().c_str());

			if (!IPAddress.IsEmpty() && !Port.IsEmpty() && !PlayerSessionID.IsEmpty())
			{
				GameLiftServerIp = IPAddress;
				GameLiftServerPort = Port;
				GameLiftPlayerSessionId = PlayerSessionID;

				FinishTaskThread(true);
			}
			else
			{
				FinishTaskThread(false);
			}

			return;
		}
	}
	else if (Status == Aws::GameLift::Model::GameSessionPlacementState::PENDING)
	{
		float TimeCollapsed = FPlatformTime::Seconds() - QuerySessionTimeStamp;
		if (TimeCollapsed < 20.0f)
		{
			if (DescribeGameSessionPlacementObject->Activate() != EActivateStatus::ACTIVATE_Success)
			{
				FinishTaskThread(false);
				return;
			}
			return;
		}
		else
		{
			//Timeout
			FinishTaskThread(false);
			return;
		}
	}
	
	FinishTaskThread(false);
}

void FOnlineAsyncTaskCreateGameliftSession::OnDescribeGameSessionPlacementFailed(const FString& ErrorMessage)
{
	FinishTaskThread(false);
}

void FOnlineAsyncTaskCreateGameliftSession::Finalize()
{

	FOnlineSessionZomboyPtr SessionInt = StaticCastSharedPtr<FOnlineSessionZomboy>(Subsystem->GetSessionInterface());
	
	if (bWasSuccessful)
	{
		if (FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName))
		{
			FOnlineSessionInfoZomboy* NewSessionInfo = new FOnlineSessionInfoZomboy();
			// Lobby sessions don't have a valid IP
			NewSessionInfo->HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr(0, 0);
			bool bIsValid;
			NewSessionInfo->HostAddr->SetIp(*GameLiftServerIp, bIsValid);
			if (!bIsValid)
			{
				bWasSuccessful = false;
				SessionInt->RemoveNamedSession(SessionName);
				return;
			}

			NewSessionInfo->HostAddr->SetPort(FCString::Atoi(*GameLiftServerPort));
			NewSessionInfo->SessionId = FUniqueNetIdString(GameLiftSessionId);
			NewSessionInfo->bIsHost = true;
			NewSessionInfo->GameLiftPlayerId = GameLiftPlayerSessionId;
			NewSessionInfo->GameLiftPlayerRegion = Subsystem->GetGameLiftClient()->GetCurrentRegion();

			Session->SessionInfo = MakeShareable(NewSessionInfo);
			Session->SessionState = EOnlineSessionState::Pending;

			SessionInt->RegisterLocalPlayers(Session);

			DumpNamedSession(Session);
		}
	}
	else
	{
		SessionInt->RemoveNamedSession(SessionName);
	}

}

void FOnlineAsyncTaskCreateGameliftSession::TriggerDelegates()
{
	IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
	if (SessionInt.IsValid())
	{
		SessionInt->TriggerOnCreateSessionCompleteDelegates(SessionName, bWasSuccessful);
	}
}

FString FOnlineAsyncTaskSearchGameliftSession::ToString() const
{
	return FString::Printf(TEXT("FOnlineAsyncTaskCreateGameliftSession bWasSuccessful: %d"), bWasSuccessful);
}

void FOnlineAsyncTaskSearchGameliftSession::TaskThreadInit()
{
	if (Subsystem)
	{
		GameLiftObject = Subsystem->GetGameLiftClient();

		if (!GameLiftObject.IsValid())
		{
			FinishTaskThread(false);
			return;
		}

		FGameLiftGameSessionSearchSettings GameLiftSearchSettings;
		GameLiftSearchSettings.SetSessionQueueName(Subsystem->GetGameLiftQueueName());

		//TODO: Implemente GameLiftServerFilter
		SearchGameSessionsObject = GameLiftObject->SearchGameLiftSessions(GameLiftSearchSettings);
		SearchGameSessionsObject->OnSearchGameSessionsSuccessDelegate.BindRaw(this, &FOnlineAsyncTaskSearchGameliftSession::OnSearchGameSessionsSuccess);
		SearchGameSessionsObject->OnSearchGameSessionsFailedDelegate.BindRaw(this, &FOnlineAsyncTaskSearchGameliftSession::OnSearchGameSessionsFailed);
		if (SearchGameSessionsObject->Activate() != EActivateStatus::ACTIVATE_Success)
		{
			FinishTaskThread(false);
		}
	}
}

void FOnlineAsyncTaskSearchGameliftSession::OnSearchGameSessionsSuccess(const TArray<FGameLiftGameSessionSearchResult>& Results)
{
	if (Results.Num() > 0 && SearchSettings.IsValid())
	{
		TArray<FOnlineSessionSearchResult> SearchResults;
		
		for (const FGameLiftGameSessionSearchResult GameLiftResult : Results)
		{
			FOnlineSessionSearchResult SearchResult;
			SearchResult.Session.NumOpenPublicConnections = GameLiftResult.MaxPlayerCount - GameLiftResult.CurrentPlayerCount;
			SearchResult.Session.SessionSettings.NumPublicConnections = GameLiftResult.MaxPlayerCount;
			
			FOnlineSessionInfoZomboy* SessionInfo = new FOnlineSessionInfoZomboy();
			SessionInfo->SessionId = FUniqueNetIdString(GameLiftResult.SessionId);
			SessionInfo->SessionRegion = GameLiftResult.SessionRegion;
			SessionInfo->HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr(0, 0);
			bool bIsValid;
			SessionInfo->HostAddr->SetIp(*GameLiftResult.IPAddress, bIsValid);
			if (!bIsValid)
			{
				continue;
			}
			SessionInfo->HostAddr->SetPort(GameLiftResult.Port);

			SearchResult.Session.SessionInfo = MakeShareable(SessionInfo);

			for (const auto& ServerDynamicProperty : GameLiftResult.DynamicProperties)
			{
				FString PropertyKey = ServerDynamicProperty.Key;
				FString PropertyValue = ServerDynamicProperty.Value;

				SearchResult.Session.SessionSettings.Set(FName(*PropertyKey), PropertyValue, EOnlineDataAdvertisementType::ViaOnlineService);
			}

			for (const auto& ServerStaticProperty : GameLiftResult.StaticProperties)
			{
				FString PropertyKey = ServerStaticProperty.Key;
				FString PropertyValue = ServerStaticProperty.Value;

				SearchResult.Session.SessionSettings.Set(FName(*PropertyKey), PropertyValue, EOnlineDataAdvertisementType::ViaPingOnly);
			}

			SearchResults.Add(SearchResult);
		}

		SearchSettings->SearchResults = SearchResults;

		FinishTaskThread(true);
		return;
	}

	FinishTaskThread(false);
}

void FOnlineAsyncTaskSearchGameliftSession::OnSearchGameSessionsFailed(const FString& ErrorMessage)
{
	FinishTaskThread(false);
}

void FOnlineAsyncTaskSearchGameliftSession::Finalize()
{

	if (bWasSuccessful)
	{
		SearchSettings->SearchState = EOnlineAsyncTaskState::Done;
	}
	else
	{
		SearchSettings->SearchState = EOnlineAsyncTaskState::Failed;
	}

	FOnlineSessionZomboyPtr SessionInt = StaticCastSharedPtr<FOnlineSessionZomboy>(Subsystem->GetSessionInterface());

	if (SessionInt->CurrentSessionSearch.IsValid() && SearchSettings == SessionInt->CurrentSessionSearch)
	{
		SessionInt->CurrentSessionSearch = NULL;
	}
}

void FOnlineAsyncTaskSearchGameliftSession::TriggerDelegates()
{
	IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
	if (SessionInt.IsValid())
	{
		SessionInt->TriggerOnFindSessionsCompleteDelegates(bWasSuccessful);
	}
}

FString FOnlineAsyncTaskJoinGameliftSession::ToString() const
{
	return FString::Printf(TEXT("FOnlineAsyncTaskJoinGameliftSession bWasSuccessful: %d"), bWasSuccessful);
}

void FOnlineAsyncTaskJoinGameliftSession::TaskThreadInit()
{
	if (Subsystem && GameSessionId.IsValid())
	{
		GameLiftObject = Subsystem->GetGameLiftClient();

		if (!GameLiftObject.IsValid())
		{
			FinishTaskThread(false);
			return;
		}

		FOnlineIdentityZomboyPtr IdentityInt = StaticCastSharedPtr<FOnlineIdentityZomboy>(Subsystem->GetIdentityInterface());

		if (IdentityInt.IsValid())
		{
			TSharedPtr<const FUniqueNetId> PlayerNetId = IdentityInt->GetUniquePlayerId(0);
			if (!PlayerNetId.IsValid())
			{
				FinishTaskThread(false);
				return;
			}

			CreatePlayerSessionObject = GameLiftObject->CreatePlayerSession(GameSessionId.ToString(), PlayerNetId->ToString(), GameSessionRegion);
			CreatePlayerSessionObject->OnCreatePlayerSessionSuccessDelegate.BindRaw(this, &FOnlineAsyncTaskJoinGameliftSession::OnCreatePlayerSessionSuccess);
			CreatePlayerSessionObject->OnCreatePlayerSessionFailedDelegate.BindRaw(this, &FOnlineAsyncTaskJoinGameliftSession::OnCreatePlayerSessionFailed);
			if (CreatePlayerSessionObject->Activate() != EActivateStatus::ACTIVATE_Success)
			{
				FinishTaskThread(false);
				return;
			}
			return;
		}
	}

	FinishTaskThread(false);
}

void FOnlineAsyncTaskJoinGameliftSession::OnCreatePlayerSessionSuccess(const FString& IPAddress, const FString& Port, const FString& PlayerSessionID)
{
	if (!IPAddress.IsEmpty() && !Port.IsEmpty() && !PlayerSessionID.IsEmpty())
	{
		FOnlineSessionZomboyPtr SessionInt = StaticCastSharedPtr<FOnlineSessionZomboy>(Subsystem->GetSessionInterface());
		if (FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName))
		{
			FOnlineSessionInfoZomboy* NewSessionInfo = new FOnlineSessionInfoZomboy();
			// Lobby sessions don't have a valid IP
			NewSessionInfo->HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr(0, 0);
			bool bIsValid;
			NewSessionInfo->HostAddr->SetIp(*IPAddress, bIsValid);
			if (!bIsValid)
			{
				FinishTaskThread(false);
				return;
			}

			NewSessionInfo->HostAddr->SetPort(FCString::Atoi(*Port));
			NewSessionInfo->SessionId = FUniqueNetIdString(GameSessionId);
			NewSessionInfo->GameLiftPlayerId = PlayerSessionID;
			NewSessionInfo->SessionRegion = GameSessionRegion;
			NewSessionInfo->GameLiftPlayerRegion = Subsystem->GetGameLiftClient()->GetCurrentRegion();


			Session->SessionInfo = MakeShareable(NewSessionInfo);
			Session->SessionState = EOnlineSessionState::Pending;

			DumpNamedSession(Session);

			FinishTaskThread(true);
			return;
		}
	}

	FinishTaskThread(false);
}

void FOnlineAsyncTaskJoinGameliftSession::OnCreatePlayerSessionFailed(const FString& ErrorMessage)
{
	FinishTaskThread(false);
}

void FOnlineAsyncTaskJoinGameliftSession::Finalize()
{
	FOnlineSessionZomboyPtr SessionInt = StaticCastSharedPtr<FOnlineSessionZomboy>(Subsystem->GetSessionInterface());
	
	if (!bWasSuccessful)
	{
		SessionInt->RemoveNamedSession(SessionName);
	}
	else
	{
		if (FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName))
		{
			SessionInt->RegisterLocalPlayers(Session);
		}
	}
}

void FOnlineAsyncTaskJoinGameliftSession::TriggerDelegates()
{
	IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
	if (SessionInt.IsValid())
	{
		SessionInt->TriggerOnJoinSessionCompleteDelegates(SessionName, bWasSuccessful ? EOnJoinSessionCompleteResult::Success : EOnJoinSessionCompleteResult::UnknownError);
	}
}

FString FOnlineAsyncTaskStartGameliftMatchmaking::ToString() const
{
	return FString::Printf(TEXT("FOnlineAsyncTaskStartGameliftMatchmaking bWasSuccessful: %d"), bWasSuccessful);
}

void FOnlineAsyncTaskStartGameliftMatchmaking::TaskThreadInit()
{
	if (Subsystem)
	{
		GameLiftObject = Subsystem->GetGameLiftClient();

		if (!GameLiftObject.IsValid())
		{
			FinishTaskThread(false);
			return;
		}

		FOnlineSessionZomboyPtr SessionInt = StaticCastSharedPtr<FOnlineSessionZomboy>(Subsystem->GetSessionInterface());
		if (FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName))
		{
			FGuid MatchmakingTicketGUID = FGuid::NewGuid();
			MatchmakingTicket = MatchmakingTicketGUID.ToString();

			FOnlineSessionInfoZomboy* NewSessionInfo = new FOnlineSessionInfoZomboy();
			NewSessionInfo->GameLiftMatchmakingTicket = MatchmakingTicket;
			NewSessionInfo->GameLiftPlayerRegion = Subsystem->GetGameLiftClient()->GetCurrentRegion();

			Session->SessionInfo = MakeShareable(NewSessionInfo);

			StartMatchmakingObject = GameLiftObject->StartMatchmaking(Subsystem->GetGameLiftMatchmakingConfigName(), PlayerIds, MatchmakingTicket, MatchmakingSettings->GameLiftPlayerAttributes);
			StartMatchmakingObject->OnStartMatchmakingSuccessDelegate.BindRaw(this, &FOnlineAsyncTaskStartGameliftMatchmaking::OnStartMatchmakingSuccess);
			StartMatchmakingObject->OnStartMatchmakingFailedDelegate.BindRaw(this, &FOnlineAsyncTaskStartGameliftMatchmaking::OnStartMatchmakingFailed);
			bMatchAccepted = false;
			if (StartMatchmakingObject->Activate() != EActivateStatus::ACTIVATE_Success)
			{
				FinishTaskThread(false);
			}
			return;
		}
	}

	FinishTaskThread(false);
}

void FOnlineAsyncTaskStartGameliftMatchmaking::OnStartMatchmakingSuccess(Aws::GameLift::Model::MatchmakingConfigurationStatus Status, const FString& InMatchmakingTicket)
{
	DescribeMatchmakingObject = GameLiftObject->DescribeMatchmaking(InMatchmakingTicket);
	DescribeMatchmakingObject->OnDescribeMatchmakingSuccessDelegate.BindRaw(this, &FOnlineAsyncTaskStartGameliftMatchmaking::OnDescribeMatchmakingSuccess);
	DescribeMatchmakingObject->OnDescribeMatchmakingFailedDelegate.BindRaw(this, &FOnlineAsyncTaskStartGameliftMatchmaking::OnDescribeMatchmakingFailed);
	if (DescribeMatchmakingObject->Activate() != EActivateStatus::ACTIVATE_Success)
	{
		FinishTaskThread(false);
	}
}

void FOnlineAsyncTaskStartGameliftMatchmaking::OnStartMatchmakingFailed(const FString& ErrorMessage)
{
	FinishTaskThread(false);
}

void FOnlineAsyncTaskStartGameliftMatchmaking::OnDescribeMatchmakingSuccess(Aws::GameLift::Model::MatchmakingConfigurationStatus Status, const FString& InGameSessionId, const FString& IPAddress, const FString& Port, const Aws::Vector<Aws::GameLift::Model::MatchedPlayerSession>& MatchedPlayerSessions, const Aws::Vector<Aws::GameLift::Model::Player>& GameLiftPlayers)
{
	if (Status == Aws::GameLift::Model::MatchmakingConfigurationStatus::COMPLETED)
	{
		if (!IPAddress.IsEmpty() && !Port.IsEmpty() && MatchedPlayerSessions.size() > 0 && MatchedPlayerSessions.size() == GameLiftPlayers.size())
		{
			FOnlineSessionZomboyPtr SessionInt = StaticCastSharedPtr<FOnlineSessionZomboy>(Subsystem->GetSessionInterface());
			if (FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName))
			{
				TSharedPtr<FOnlineSessionInfoZomboy> SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoZomboy>(Session->SessionInfo);

				// Lobby sessions don't have a valid IP
				SessionInfo->HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr(0, 0);
				bool bIsValid;
				SessionInfo->HostAddr->SetIp(*IPAddress, bIsValid);
				if (!bIsValid)
				{
					FinishTaskThread(false);
					SessionInt->RemoveNamedSession(SessionName);
					return;
				}

				SessionInfo->HostAddr->SetPort(FCString::Atoi(*Port));
				SessionInfo->SessionId = FUniqueNetIdString(InGameSessionId);

				SessionInfo->bFromMatchmaking = true;
				SessionInfo->GameLiftPlayerId = FString(MatchedPlayerSessions[0].GetPlayerSessionId().c_str());
				Session->SessionState = EOnlineSessionState::Pending;

				DumpNamedSession(Session);

				FinishTaskThread(true);
				return;
			}
		}
	}
	else
	{
		if (Status == Aws::GameLift::Model::MatchmakingConfigurationStatus::REQUIRES_ACCEPTANCE)
		{
			UE_LOG(LogTemp, Log, TEXT("MatchmakingConfigurationStatus:REQUIRES_ACCEPTANCE"))

			if (!bMatchAccepted)
			{
				if (StartMatchmakingObject.IsValid())
				{
					StartMatchmakingObject->AcceptMatch();
					bMatchAccepted = true;
				}
			}
		}
		else
		{
			bMatchAccepted = false;
		}

		if (DescribeMatchmakingObject->Activate() != EActivateStatus::ACTIVATE_Success)
		{
			FinishTaskThread(false);
		}
		return;
	}

	FinishTaskThread(false);
}

void FOnlineAsyncTaskStartGameliftMatchmaking::OnDescribeMatchmakingFailed(const FString& ErrorMessage, Aws::GameLift::Model::MatchmakingConfigurationStatus Status, bool bCriticalFailure)
{
	if (Status == Aws::GameLift::Model::MatchmakingConfigurationStatus::FAILED
		|| Status == Aws::GameLift::Model::MatchmakingConfigurationStatus::TIMED_OUT)
	{
		if (StartMatchmakingObject.IsValid())
		{
			FOnlineSessionZomboyPtr SessionInt = StaticCastSharedPtr<FOnlineSessionZomboy>(Subsystem->GetSessionInterface());
			if (FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName))
			{
				TSharedPtr<FOnlineSessionInfoZomboy> ZomboySessionInfo = StaticCastSharedPtr<FOnlineSessionInfoZomboy>(Session->SessionInfo);
				if (ZomboySessionInfo.IsValid())
				{
					FGuid MatchmakingTicketGUID = FGuid::NewGuid();
					MatchmakingTicket = MatchmakingTicketGUID.ToString();

					ZomboySessionInfo->GameLiftMatchmakingTicket = MatchmakingTicket;

					StartMatchmakingObject->ChangeMatchmakingTicket(MatchmakingTicket);
					UE_LOG(LogTemp, Log, TEXT("Resubmiting matchmaking request."));

					if (StartMatchmakingObject->Activate() != EActivateStatus::ACTIVATE_Success)
					{
						FinishTaskThread(false);
					}
					bMatchAccepted = false;

					return;
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("OnDescribeMatchmakingFailed: %s"), *ErrorMessage);
	FinishTaskThread(false);
}

void FOnlineAsyncTaskStartGameliftMatchmaking::Finalize()
{
	FOnlineSessionZomboyPtr SessionInt = StaticCastSharedPtr<FOnlineSessionZomboy>(Subsystem->GetSessionInterface());

	if (!bWasSuccessful)
	{
		SessionInt->RemoveNamedSession(SessionName);
	}
	else
	{
		if (FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName))
		{
			SessionInt->RegisterLocalPlayers(Session);
		}
	}
}

void FOnlineAsyncTaskStartGameliftMatchmaking::TriggerDelegates()
{
	IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
	if (SessionInt.IsValid())
	{
		SessionInt->TriggerOnMatchmakingCompleteDelegates(SessionName, bWasSuccessful);
	}
}

FString FOnlineAsyncTaskStopGameliftMatchmaking::ToString() const
{
	return FString::Printf(TEXT("FOnlineAsyncTaskStopGameliftMatchmaking bWasSuccessful: %d"), bWasSuccessful);
}

void FOnlineAsyncTaskStopGameliftMatchmaking::TaskThreadInit()
{
	if (Subsystem)
	{
		GameLiftObject = Subsystem->GetGameLiftClient();

		if (!GameLiftObject.IsValid())
		{
			FinishTaskThread(false);
			return;
		}

		FOnlineSessionZomboyPtr SessionInt = StaticCastSharedPtr<FOnlineSessionZomboy>(Subsystem->GetSessionInterface());
		if (FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName))
		{
			TSharedPtr<FOnlineSessionInfoZomboy> SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoZomboy>(Session->SessionInfo);
			if (SessionInfo.IsValid() && !SessionInfo->GameLiftMatchmakingTicket.IsEmpty())
			{
				FString MatchmakingTicker = SessionInfo->GameLiftMatchmakingTicket;

				StopMatchmakingObject = GameLiftObject->StopMatchmaking(MatchmakingTicker);
				StopMatchmakingObject->OnStopGameLiftMatchmakingSuccessDelegate.BindRaw(this, &FOnlineAsyncTaskStopGameliftMatchmaking::OnSotpMatchmakingSuccess);
				StopMatchmakingObject->OnStopGameLiftMatchmakingFailedDelegate.BindRaw(this, &FOnlineAsyncTaskStopGameliftMatchmaking::OnSotpMatchmakingFailed);
				if (StopMatchmakingObject->Activate() == EActivateStatus::ACTIVATE_Success)
				{
					return;
				}
			}
		}
	}

	FinishTaskThread(false);
}

void FOnlineAsyncTaskStopGameliftMatchmaking::OnSotpMatchmakingSuccess()
{
	FinishTaskThread(true);
}

void FOnlineAsyncTaskStopGameliftMatchmaking::OnSotpMatchmakingFailed()
{
	FinishTaskThread(false);
}

void FOnlineAsyncTaskStopGameliftMatchmaking::Finalize()
{
	FOnlineSessionZomboyPtr SessionInt = StaticCastSharedPtr<FOnlineSessionZomboy>(Subsystem->GetSessionInterface());

	if (!bWasSuccessful)
	{
		SessionInt->RemoveNamedSession(SessionName);
	}
}

void FOnlineAsyncTaskStopGameliftMatchmaking::TriggerDelegates()
{
	IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
	if (SessionInt.IsValid())
	{
		SessionInt->TriggerOnCancelMatchmakingCompleteDelegates(SessionName, bWasSuccessful);
	}
}

#endif
