// Fill out your copyright notice in the Description page of Project Settings.

#include "GameLiftServerOnlineSession.h"
#include "OnlineSubsystemZomboy.h"

#include "Async.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "EngineUtils.h"
#include "Engine.h"
#include "GameFramework/GameSession.h"


UGameLiftServerOnlineSession* UGameLiftServerOnlineSession::GetServerOnlineSession(const UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		UGameLiftServerOnlineSession* ServerOnlineSession = CastChecked<UGameLiftServerOnlineSession>(World->GetGameInstance()->GetOnlineSession());
		return ServerOnlineSession;
	}
	return NULL;
}

UGameInstance* UGameLiftServerOnlineSession::GetGameInstance() const
{
	return CastChecked<UGameInstance>(GetOuter());
}

UWorld* UGameLiftServerOnlineSession::GetWorld() const
{
	return GetGameInstance()->GetWorld();
}

void UGameLiftServerOnlineSession::InitGameLiftServices()
{
	if (bInitialized)
		return;
	
	bInitialized = true;

	int CmdIsHostingLan = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("HostLan="), CmdIsHostingLan))
	{
		bIsHostingLan = (CmdIsHostingLan == 1);
	}

	if (!bIsHostingLan)
	{
#if WITH_GAMELIFT_SERVER
		//Getting the module first.
		GameLiftSdkModule = MakeShareable(&FModuleManager::LoadModuleChecked<FGameLiftServerSDKModule>(FName("GameLiftServerSDK")));

		////InitSDK establishes a local connection with GameLift's agent to enable communication.
		GameLiftSdkModule->InitSDK();

		FProcessParameters* params = new FProcessParameters();
		params->OnStartGameSession.BindUObject(this, &UGameLiftServerOnlineSession::AsyncOnServiceActivate);
		params->OnUpdateGameSession.BindUObject(this, &UGameLiftServerOnlineSession::AsyncOnServiceUpdate);
		params->OnTerminate.BindUObject(this, &UGameLiftServerOnlineSession::AsyncOnServiceTerminate);
		params->OnHealthCheck.BindUObject(this, &UGameLiftServerOnlineSession::AsyncOnServiceHealthCheck);
		params->port = GetWorld()->URL.Port;

		//Here, the game server tells GameLift what set of files to upload when the game session 
		//ends. GameLift uploads everything specified here for the developers to fetch later.
		TArray<FString> logfiles;
		logfiles.Add(FPaths::ProjectLogDir());
		params->logParameters = logfiles;

		//Call ProcessReady to tell GameLift this game server is ready to receive game sessions!
		GameLiftSdkModule->ProcessReady(*params);
#endif
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Hosting Lan....."));

		if (FOnlineSubsystemZomboy* OnlineSubZomboy = StaticCast<FOnlineSubsystemZomboy*>(IOnlineSubsystem::Get(FName("Zomboy"))))
		{
			IOnlineSessionPtr Sessions = OnlineSubZomboy->GetSessionInterface();
			if (Sessions.IsValid())
			{
				CurrentGameLiftServerSession.MaxPlayers = 10;
				TMap<FName, FString> LanHostProperties;
				LanHostProperties.Add(TEXT("mp"), TEXT("lobby"));
				LanHostProperties.Add(TEXT("md"), TEXT("lobby"));
				CurrentGameLiftServerSession.DynamicProperties = LanHostProperties;

				TSharedPtr<FOnlineSessionSettings> HostSettings = MakeShareable(new FOnlineSessionSettings());
				HostSettings->bIsLANMatch = true;
				HostSettings->bUsesPresence = true;
				HostSettings->NumPublicConnections = CurrentGameLiftServerSession.MaxPlayers;
				HostSettings->NumPrivateConnections = 0;
				HostSettings->bAllowJoinInProgress = true;
				HostSettings->bAllowJoinViaPresence = true;
				HostSettings->bShouldAdvertise = true;

				if (!Sessions->CreateSession(0, GameSessionName, *HostSettings))
				{
					UE_LOG(GameLiftLog, Log, TEXT("UGameLiftServerOnlineSession::OnServiceReady: Failed to create game session"));
				}

				//ServerChangeMap(LanHostProperties);
				FString ServerTravelURL = GetServerTravelURL(CurrentGameLiftServerSession);
				ServerTravelURL += TEXT("?listen");
				UGameplayStatics::OpenLevel(GetOuter(), *ServerTravelURL, true);
			}
		}
	}


	UE_LOG(LogTemp, Log, TEXT("----------------InitGameLiftService with port: %d"), GetWorld()->URL.Port);
}

void UGameLiftServerOnlineSession::TerminateGameSession()
{

#if WITH_GAMELIFT_SERVER
	if (!bIsHostingLan)
	{
		PlayerTeamMap.Reset();

		if (FOnlineSubsystemZomboy* OnlineSubZomboy = StaticCast<FOnlineSubsystemZomboy*>(IOnlineSubsystem::Get(FName("Zomboy"))))
		{
			OnlineSubZomboy->GetServerAsyncTaskManager()->AddBackfillStopRequest(GameLiftSdkModule, CurrentGameLiftServerSession.SessionId, CurrentGameLiftServerSession.MatchmakingConfigurationArn);
		}

		GameLiftSdkModule->TerminateGameSession();

		if (FOnlineSubsystemZomboy* OnlineSubZomboy = StaticCast<FOnlineSubsystemZomboy*>(IOnlineSubsystem::Get(FName("Zomboy"))))
		{
			IOnlineSessionPtr Sessions = OnlineSubZomboy->GetSessionInterface();
			if (Sessions.IsValid())
			{
				if (!Sessions->DestroySession(GameSessionName))
				{
					UE_LOG(GameLiftLog, Log, TEXT("UGameLiftServerOnlineSession::OnServiceReady: Failed to destroy game session"));
				}
			}
		}

		// if already loaded, do nothing
		UWorld* const World = GetWorld();
		if (World)
		{
			FString const CurrentMapName = *World->PersistentLevel->GetOutermost()->GetName();
			if (CurrentMapName == DedicatedServerMap)
			{
				return;
			}
		}

		FString Error;
		EBrowseReturnVal::Type BrowseRet = EBrowseReturnVal::Failure;
		FURL URL(
			*FString::Printf(TEXT("%s"), *DedicatedServerMap)
		);

		if (URL.Valid && !HasAnyFlags(RF_ClassDefaultObject)) //CastChecked<UEngine>() will fail if using Default__ShooterGameInstance, so make sure that we're not default
		{
			UGameplayStatics::OpenLevel(GetWorld(), *DedicatedServerMap, true);
		}
	}

#endif
}

#if WITH_GAMELIFT_SERVER

void UGameLiftServerOnlineSession::AsyncOnServiceActivate(Aws::GameLift::Server:: Model::GameSession GameSession)
{
	UE_LOG(LogTemp, Log, TEXT("----------------AsyncOnServiceActivate with port: %d"), GetWorld()->URL.Port);

	FGameLiftServerGameSession GameLiftGameSession;

	GameLiftGameSession.SessionId = GameSession.GetGameSessionId();
	GameLiftGameSession.MaxPlayers = GameSession.GetMaximumPlayerSessionCount();
	
	TSharedPtr<FJsonObject> GameSessioNameDataObject = MakeShareable(new FJsonObject());
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(UTF8_TO_TCHAR(GameSession.GetName()));

	if (FJsonSerializer::Deserialize(JsonReader, GameSessioNameDataObject) && GameSessioNameDataObject.IsValid())
	{
		for (auto& GameSessionProperty : GameSessioNameDataObject->Values)
		{
			FString Key = GameSessionProperty.Key;
			if (Key.IsEmpty()) continue;

			TSharedPtr<FJsonValue> JsonValue = GameSessionProperty.Value;
			FString Value;
			if (JsonValue.IsValid() && JsonValue->TryGetString(Value))
			{
				GameLiftGameSession.DynamicProperties.Add(*Key, Value);
			}
		}
	}

	int NumProperties = 0;
	auto Properties = GameSession.GetGameProperties(NumProperties);

	for (int32 i = 0; i < NumProperties; i++)
	{
		const Aws::GameLift::Server::Model::GameProperty& Property = Properties[i];

		FName Key = UTF8_TO_TCHAR(Property.GetKey());
		FString Value = UTF8_TO_TCHAR(Property.GetValue());

		UE_LOG(LogTemp, Log, TEXT("Added Property with value: %s"), *Value);

		GameLiftGameSession.StaticProperties.Add(Key, Value);
	}

	GameLiftGameSession.MatchmakingData = FString(UTF8_TO_TCHAR(GameSession.GetMatchmakerData()));
	GameLiftGameSession.bIsMatchmakingServer = !GameLiftGameSession.MatchmakingData.IsEmpty();

	AsyncTask(ENamedThreads::GameThread, [=]()
	{
		OnServiceReady(GameLiftGameSession);
	});
}

void UGameLiftServerOnlineSession::AsyncOnServiceUpdate(Aws::GameLift::Server::Model::UpdateGameSession UpdateGameSession)
{
	AsyncTask(ENamedThreads::GameThread, [=]()
	{
		UE_LOG(LogTemp, Log, TEXT("----------------AsyncOnServiceUpdate with reason: %d"), (int)UpdateGameSession.GetUpdateReason());
		if (UpdateGameSession.GetUpdateReason() != Aws::GameLift::Server::Model::UpdateReason::BACKFILL_CANCELLED)
		{
			FString MatchmakerDataString = FString(UpdateGameSession.GetGameSession().GetMatchmakerData());
			UE_LOG(LogTemp, Log, TEXT("----------------AsyncOnServiceUpdate with Matchmaker Data: %s"), *MatchmakerDataString);

			if (UpdateGameSession.GetUpdateReason() == Aws::GameLift::Server::Model::UpdateReason::BACKFILL_FAILED)
			{
				UE_LOG(LogTemp, Log, TEXT("----------------AsyncOnServiceUpdate Failed"));

				RequestBackfill(true);
			}
			else
			{
				if (UpdateGameSession.GetUpdateReason() == Aws::GameLift::Server::Model::UpdateReason::MATCHMAKING_DATA_UPDATED)
				{
					UE_LOG(LogTemp, Log, TEXT("----------------AsyncOnServiceUpdate Matchmaker Data Updated"));

					BuildPlayerMatchmakingReservation(MatchmakerDataString);
				}
				else
				{
					RequestBackfill();
				}

				if (UpdateGameSession.GetUpdateReason() == Aws::GameLift::Server::Model::UpdateReason::BACKFILL_TIMED_OUT)
				{
					UE_LOG(LogTemp, Log, TEXT("----------------AsyncOnServiceUpdate Request new backfill due to timeout."));
				}

			}
		}
	
	});

}

void UGameLiftServerOnlineSession::AsyncOnServiceTerminate()
{
	AsyncTask(ENamedThreads::GameThread, [=]()
	{
		UE_LOG(GameLiftLog, Log, TEXT("GameLift OnProcessTerminate, shutting down the server"));

		//Aws::GameLift::GenericOutcome Outcome = Aws::GameLift::Server::ProcessEnding();
		GameLiftSdkModule->ProcessEnding();

		AGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AGameModeBase>();

		if (GameMode)
		{
			if (GameMode->GameSession)
			{
				const FText KickText = FText::FromString(TEXT("Server is shutting down"));

				for (TActorIterator<APlayerController> Itr(GetWorld()); Itr; ++Itr)
				{
					GameMode->GameSession->KickPlayer(*Itr, KickText);
				}
			}
		}
		
		//Shutdown Dedicated Server
		FGenericPlatformMisc::RequestExit(false);
	});
}

bool UGameLiftServerOnlineSession::AsyncOnServiceHealthCheck()
{
	return true;
}

void UGameLiftServerOnlineSession::OnServiceReady(const FGameLiftServerGameSession& GameLiftGameSession)
{
	UE_LOG(LogTemp, Log, TEXT("On Service Ready--------------"));

	CurrentGameLiftServerSession = GameLiftGameSession;

	PlayerSessions.Empty();

	UE_LOG(GameLiftLog, Log, TEXT("Session ID: %s"), *CurrentGameLiftServerSession.SessionId);
	SetServerRegion(CurrentGameLiftServerSession.SessionId);
	FString MatchmakingData = CurrentGameLiftServerSession.MatchmakingData;
	UE_LOG(GameLiftLog, Log, TEXT("Match maker data: %s"), *MatchmakingData);
	BuildPlayerMatchmakingReservation(MatchmakingData);

	ProcessGameSessionReady(CurrentGameLiftServerSession);

	//Server travel to map
	FString ServerTravelURL = GetServerTravelURL(CurrentGameLiftServerSession);
	ServerTravelURL += TEXT("?listen");
	UGameplayStatics::OpenLevel(GetOuter(), *ServerTravelURL, true);

	if (FOnlineSubsystemZomboy* OnlineSubZomboy = StaticCast<FOnlineSubsystemZomboy*>(IOnlineSubsystem::Get(FName("Zomboy"))))
	{
		OnlineSubZomboy->SetGameLiftClientRegion(CurrentGameLiftServerSession.Region);

		AdvertiseServerProperties();

		//Create new game session
		IOnlineSessionPtr Sessions = OnlineSubZomboy->GetSessionInterface();
		if (Sessions.IsValid())
		{
			TSharedPtr<FOnlineSessionSettings> HostSettings = MakeShareable(new FOnlineSessionSettings());
			HostSettings->bIsLANMatch = false;
			HostSettings->bUsesPresence = false;
			HostSettings->NumPublicConnections = CurrentGameLiftServerSession.MaxPlayers;
			HostSettings->NumPrivateConnections = 0;

			if (!Sessions->CreateSession(0, GameSessionName, *HostSettings))
			{
				UE_LOG(GameLiftLog, Log, TEXT("UGameLiftServerOnlineSession::OnServiceReady: Failed to create game session"));
			}
		}
	}

	//Activate GameLift GameSession
	GameLiftSdkModule->ActivateGameSession();
}

#endif

bool UGameLiftServerOnlineSession::AcceptPlayerSession(const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{	
	if (bIsHostingLan)
		return true;

	double TimeStamp = FPlatformTime::Seconds();

	const FString PlayerSessionId = UGameplayStatics::ParseOption(Options, GAMELIFT_URL_PLAYER_SESSION_ID);

	UE_LOG(GameLiftLog, Log, TEXT("GameLift::Server::AcceptPlayerSession with player session id %s"), *PlayerSessionId);

#if WITH_GAMELIFT_SERVER
	FGameLiftGenericOutcome ConnectOutcome = GameLiftSdkModule->AcceptPlayerSession(PlayerSessionId);

	if (!ConnectOutcome.IsSuccess())
	{
		//We dont want to send the gamelift error messages to users, just log it in the backend
		ErrorMessage = TEXT("Internal error");

		const FString LogErrorMessage = ConnectOutcome.GetError().m_errorMessage;

		UE_LOG(GameLiftLog, Warning, TEXT("GameLift::Server::AcceptPlayerSession failed with error %s"), *LogErrorMessage);
		return false;
	}

	const FString UniqueIdStr = UniqueId.ToString();

	if (UniqueIdStr.Len() <= 0)
	{
		return false;
	}

	UE_LOG(GameLiftLog, Log, TEXT("GameLift::Server::AcceptPlayerSession with player net id %s"), *UniqueIdStr);


	if (PlayerSessions.Find(UniqueIdStr))
	{
		UE_LOG(GameLiftLog, Warning, TEXT("GameLift AcceptPlayerSession UniqueId %s has multiple sessions ids, checking for duplicates"), *UniqueIdStr);
	}
		
	TArray<FString>& SavedSessions = PlayerSessions.Add(UniqueIdStr);

	if (SavedSessions.Find(PlayerSessionId) == INDEX_NONE)
	{
		//Remove all other player sessions, that could left over due to a crash
		for (const FString& SessionId : SavedSessions)
		{
			FGameLiftGenericOutcome DisconnectOutcome = GameLiftSdkModule->RemovePlayerSession(SessionId);
		}

		SavedSessions.Empty();

		SavedSessions.Add(PlayerSessionId);
	}
	else
	{
		UE_LOG(GameLiftLog, Warning, TEXT("GameLift AcceptPlayerSession UniqueId %s has a duplicate player session, dropping connection"), *UniqueIdStr);
		return false;
	}
#endif

	double TimeCollapsed = FPlatformTime::Seconds() - TimeStamp;

	UE_LOG(LogTemp, Log, TEXT("+++++++++++++++AcceptPlayerSession used %lf"), TimeCollapsed);

	return true;
}

void UGameLiftServerOnlineSession::RemovePlayerSession(const FUniqueNetIdRepl& UniqueId)
{
	if (bIsHostingLan)
		return;

	double TimeStamp = FPlatformTime::Seconds();

	if (FOnlineSubsystemZomboy* OnlineSubZomboy = StaticCast<FOnlineSubsystemZomboy*>(IOnlineSubsystem::Get(FName("Zomboy"))))
	{
		const FString UniqueIdStr = UniqueId.ToString();

		const TArray<FString>* pSavedSessions = PlayerSessions.Find(UniqueId.ToString());

		//if a player joined with an unique id, it gotta leave with the same id
		ensure(pSavedSessions != nullptr);

		if (!pSavedSessions)
		{
			UE_LOG(GameLiftLog, Warning, TEXT("GameLift::Server::RemovePlayerSession UniqueId not found %s"), *UniqueIdStr);
		}
		else
		{
			ensure((*pSavedSessions).Num() == 1);
			for (const FString& SessionId : (*pSavedSessions))
			{
#if WITH_GAMELIFT_SERVER
				UE_LOG(GameLiftLog, Log, TEXT("GameLift::Server::RemovePlayerSession with player session id %s"), *SessionId);

				OnlineSubZomboy->GetServerAsyncTaskManager()->AddRemovePlayerSessionRequest(GameLiftSdkModule, SessionId);
#endif
			}

			RequestBackfill();
		}
	}

	double TimeCollapsed = FPlatformTime::Seconds() - TimeStamp;

	UE_LOG(LogTemp, Log, TEXT("+++++++++++++++RemovePlayerSession used %lf"), TimeCollapsed);
}



void UGameLiftServerOnlineSession::SetServerRegion(const FString& SessionID)
{
	//Get Region String
	FString DestinationArn = SessionID;
	TArray<FString> DestinationParseArray;
	DestinationArn.ParseIntoArray(DestinationParseArray, TEXT("::gamesession/"), true);
	if (DestinationParseArray.Num() > 1)
	{
		FString Head = DestinationParseArray[0];
		FString Alias = DestinationParseArray[1];


		TArray<FString> RegionParseArray;
		Head.ParseIntoArray(RegionParseArray, TEXT("gamelift:"), true);
		if (RegionParseArray.Num() > 1)
		{
			FString RegionString = RegionParseArray[1];
			EGameLiftRegion GameLiftRegion = GetRegion(*RegionString);
			CurrentGameLiftServerSession.Region = GameLiftRegion;
		}
	}
}

void UGameLiftServerOnlineSession::BuildPlayerMatchmakingReservation(const FString& MatchmakingData)
{
	//PlayerTeamMap.Empty();

	TSharedPtr<FJsonObject> MatchmakingDataObject = MakeShareable(new FJsonObject());
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(MatchmakingData);

	if (FJsonSerializer::Deserialize(JsonReader, MatchmakingDataObject) && MatchmakingDataObject.IsValid())
	{
		FString TempMatchmakingConfigurationArn;
		if (MatchmakingDataObject->TryGetStringField(TEXT("matchmakingConfigurationArn"), TempMatchmakingConfigurationArn))
		{
			CurrentGameLiftServerSession.MatchmakingConfigurationArn = TempMatchmakingConfigurationArn;

			const TArray<TSharedPtr<FJsonValue>>* TeamJsonFieldArray;
			if (MatchmakingDataObject->TryGetArrayField(TEXT("teams"), TeamJsonFieldArray))
			{
				for (const TSharedPtr<FJsonValue>& TeamJsonFiled : *TeamJsonFieldArray)
				{
					TSharedPtr<FJsonObject> TeamObject = TeamJsonFiled->AsObject();
					if (TeamObject.IsValid())
					{
						FString TeamName;
						if (TeamObject->TryGetStringField(TEXT("name"), TeamName))
						{
							UE_LOG(LogTemp, Log, TEXT("Team Name: %s"), *TeamName);

							TArray<FUniqueNetIdZomboyPlayer>& TeamPlayers = PlayerTeamMap.FindOrAdd(TeamName);
							TeamPlayers.Empty();

							const TArray<TSharedPtr<FJsonValue>>* PlayerJsonFieldArray;
							if (TeamObject->TryGetArrayField(TEXT("players"), PlayerJsonFieldArray))
							{
								for (const TSharedPtr<FJsonValue>& PlayerJsonField : *PlayerJsonFieldArray)
								{
									TSharedPtr<FJsonObject> PlayerJsonObject = PlayerJsonField->AsObject();
									if (PlayerJsonObject.IsValid())
									{
										FString PlayerId;
										if (PlayerJsonObject->TryGetStringField(TEXT("playerId"), PlayerId))
										{
											UE_LOG(LogTemp, Log, TEXT("Player ID: %s"), *PlayerId);
											TeamPlayers.Add(FUniqueNetIdZomboyPlayer(PlayerId));
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

int UGameLiftServerOnlineSession::GetPlayerTeamNumber(class APlayerState* PlayerState)
{
	return 0;
}

EGameLiftRegion UGameLiftServerOnlineSession::GetPlayerRegion(class APlayerState* PlayerState)
{
	return EGameLiftRegion::EGameLiftRegion_NA;
}



TMap<FString, FClientAttributeValue> UGameLiftServerOnlineSession::GetPlayerAttributes(class APlayerState* PlayerState)
{
	TMap<FString, FClientAttributeValue> Empty;
	return Empty;
}

void UGameLiftServerOnlineSession::ProcessGameSessionReady(FGameLiftServerGameSession& GameLiftGameSession)
{

}

FString UGameLiftServerOnlineSession::GetServerTravelURL(const FGameLiftServerGameSession& GameLiftGameSession)
{
	FString TravelURL = GameLiftGameSession.GetGameSessionDynamicProperty(GAMELIFT_PROPERTY_MAP_KEY);

	if (!TravelURL.IsEmpty())
	{
		TravelURL += FString::Printf(TEXT("?maxPlayers=%d"), GameLiftGameSession.MaxPlayers);

		for (auto PropertyItem : GameLiftGameSession.DynamicProperties)
		{
			if (PropertyItem.Key == GAMELIFT_PROPERTY_MAP_KEY) continue;

			if (PropertyItem.Key == NAME_None || PropertyItem.Value.IsEmpty()) continue;

			TravelURL += "?";
			TravelURL += PropertyItem.Key.ToString();
			TravelURL += "=";
			TravelURL += PropertyItem.Value;
		}

		UE_LOG(LogTemp, Log, TEXT("GameLiftSession with URL = %s"), *TravelURL);
	}

	return TravelURL;
}

void UGameLiftServerOnlineSession::GetAdvertisedDynamicServerProperties(const FGameLiftServerGameSession& GameLiftGameSession, TMap<FName, FString>& OutPropertiesToAdvertise)
{
	OutPropertiesToAdvertise = GameLiftGameSession.DynamicProperties;
}

void UGameLiftServerOnlineSession::RequestBackfill(bool bFromFailure)
{
	if (!CurrentGameLiftServerSession.bIsMatchmakingServer)
	{
		UE_LOG(LogTemp, Log, TEXT("UGameLiftServerOnlineSession::RequestBackfill Abandon backfill request since this is not a matchmaking server."));
		return;
	}

	FTimerManager& TimeManager = GetGameInstance()->GetTimerManager();
	float TimeRemaining = 10000.0f;

#if WITH_GAMELIFT_SERVER

	if (FOnlineSubsystemZomboy* OnlineSubZomboy = StaticCast<FOnlineSubsystemZomboy*>(IOnlineSubsystem::Get(FName("Zomboy"))))
	{
		OnlineSubZomboy->GetServerAsyncTaskManager()->AddBackfillStopRequest(GameLiftSdkModule, CurrentGameLiftServerSession.SessionId, CurrentGameLiftServerSession.MatchmakingConfigurationArn);
	}

#endif

	if (TimerHandle_RequestBackfillTimer.IsValid())
	{
		TimeRemaining = TimeManager.GetTimerRemaining(TimerHandle_RequestBackfillTimer);
	}

	if (!bInBackfillFailure)
	{
		if (TimerHandle_RequestBackfillTimer.IsValid())
		{
			TimeManager.ClearTimer(TimerHandle_RequestBackfillTimer);
		}

		if (!bFromFailure)
		{
			TimeManager.SetTimer(TimerHandle_RequestBackfillTimer, this, &UGameLiftServerOnlineSession::TryStartMatchmakingBackfillRequest, 5.0f, false);
		}
		else
		{
			bInBackfillFailure = true;
			TimeManager.SetTimer(TimerHandle_RequestBackfillTimer, this, &UGameLiftServerOnlineSession::TryStartMatchmakingBackfillRequest, 20.0f, false);
		}
	}
	else
	{
		if (!bFromFailure)
		{
			if (TimeRemaining < 5.0f)
			{
				TimeManager.ClearTimer(TimerHandle_RequestBackfillTimer);
				TimeManager.SetTimer(TimerHandle_RequestBackfillTimer, this, &UGameLiftServerOnlineSession::TryStartMatchmakingBackfillRequest, 5.0f, false);
			}
		}
		else
		{
			if (TimerHandle_RequestBackfillTimer.IsValid())
			{
				TimeManager.ClearTimer(TimerHandle_RequestBackfillTimer);
			}

			bInBackfillFailure = true;
			TimeManager.SetTimer(TimerHandle_RequestBackfillTimer, this, &UGameLiftServerOnlineSession::TryStartMatchmakingBackfillRequest, 20.0f, false);
		}
	}
}

void UGameLiftServerOnlineSession::ServerChangeDynamicProperties(const TMap<FName, FString>& NewGameProperties)
{
	for (auto PropertyItem : NewGameProperties)
	{
		if (PropertyItem.Key == NAME_None || PropertyItem.Value.IsEmpty()) continue;

		CurrentGameLiftServerSession.DynamicProperties.Add(PropertyItem.Key, PropertyItem.Value);
	}

	if (!bIsHostingLan)
	{
		AdvertiseServerProperties();
	}
}

void UGameLiftServerOnlineSession::ServerChangeMap(const TMap<FName, FString>& NewGameProperties)
{
	ServerChangeDynamicProperties(NewGameProperties);

	FString ServerTravelURL = GetServerTravelURL(CurrentGameLiftServerSession);

	if (GetWorld())
	{
		GetWorld()->ServerTravel(ServerTravelURL, false);	
	}
}

#if WITH_GAMELIFT_SERVER

FAttributeValue ToServerAttribute(const FClientAttributeValue& Attribute)
{
	FAttributeValue ServerAttribute;
	ServerAttribute.m_N = Attribute.m_N;
	ServerAttribute.m_S = Attribute.m_S;
	ServerAttribute.m_SL = Attribute.m_SL;
	ServerAttribute.m_SDM = Attribute.m_SDM;
	ServerAttribute.m_type = (FAttributeType)Attribute.m_type;

	return ServerAttribute;
}

#endif


void UGameLiftServerOnlineSession::TryStartMatchmakingBackfillRequest()
{
	if (bIsHostingLan)
		return;

	double TimeStamp = FPlatformTime::Seconds();

	bInBackfillFailure = false;
		
#if WITH_GAMELIFT_SERVER


	TArray<FString> TeamNames;
	PlayerTeamMap.GenerateKeyArray(TeamNames);

	if (FOnlineSubsystemZomboy* OnlineSubZomboy = StaticCast<FOnlineSubsystemZomboy*>(IOnlineSubsystem::Get(FName("Zomboy"))))
	{
		if (AGameStateBase* GameState = GetGameInstance()->GetWorld()->GetGameState())
		{
			if (GameState->PlayerArray.Num() >= CurrentGameLiftServerSession.MaxPlayers || GameState->PlayerArray.Num() <= 0)
			{
				UE_LOG(LogTemp, Log, TEXT("TryStartMatchmakingBackfillRequest: Failed to start backfill."));
				return;
			}

			FGuid MatchmakingTicketGUID = FGuid::NewGuid();

			TArray<FPlayer> MatchmakingPlayers;

			for (APlayerState* PlayerState : GameState->PlayerArray)
			{
				FPlayer MatchmakingPlayer;
				MatchmakingPlayer.m_playerId = PlayerState->UniqueId.ToString();

				int PlayerTeamNumber = GetPlayerTeamNumber(PlayerState);

				if (TeamNames.IsValidIndex(PlayerTeamNumber))
				{
					MatchmakingPlayer.m_team = TeamNames[PlayerTeamNumber];
				}
				else
				{
					MatchmakingPlayer.m_team = TEXT("blue");
				}

				UE_LOG(LogTemp, Log, TEXT("tryStartMatchmakingBackfillRequest, Player in %s team."), *MatchmakingPlayer.m_team);

				MatchmakingPlayer.m_latencyInMs = GetRegionLatency(GetPlayerRegion(PlayerState));

				TMap<FString, FClientAttributeValue> PlayerAttribuates = GetPlayerAttributes(PlayerState);
				for (const auto& PlayerAttribute : PlayerAttribuates)
				{
					FAttributeValue ServerAttributeValue = ToServerAttribute(PlayerAttribute.Value);
					MatchmakingPlayer.m_playerAttributes.Add(PlayerAttribute.Key, ServerAttributeValue);
				}


				MatchmakingPlayers.Add(MatchmakingPlayer);
			}

			OnlineSubZomboy->GetServerAsyncTaskManager()->AddBackfillStartRequest(GameLiftSdkModule, CurrentGameLiftServerSession.SessionId, CurrentGameLiftServerSession.MatchmakingConfigurationArn, MatchmakingPlayers, CurrentGameLiftServerSession.MaxPlayers);
		}
	}

#endif

	double TimeCollapsed = FPlatformTime::Seconds() - TimeStamp;

	UE_LOG(LogTemp, Log, TEXT("+++++++++++++++TryStartMatchmakingBackfillRequest used %lf"), TimeCollapsed);
}

void UGameLiftServerOnlineSession::AdvertiseServerProperties()
{
	if (CurrentGameLiftServerSession.SessionId.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("UGameLiftServerOnlineSession::AdvertiseServerProperties: No valid session Id."));
		return;
	}

	if (FOnlineSubsystemZomboy* OnlineSubZomboy = StaticCast<FOnlineSubsystemZomboy*>(IOnlineSubsystem::Get(FName("Zomboy"))))
	{
		TMap<FName, FString> PropertiesToAdvertise;
		GetAdvertisedDynamicServerProperties(CurrentGameLiftServerSession, PropertiesToAdvertise);

		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

		for (auto Property : PropertiesToAdvertise)
		{
			if (Property.Key == NAME_None || Property.Value.IsEmpty()) continue;

			JsonObject->SetStringField(Property.Key.ToString(), Property.Value);
		}

		FString OutputString;
		TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

		OnlineSubZomboy->UpdateSession(CurrentGameLiftServerSession.SessionId, OutputString);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UGameLiftServerOnlineSession::AdvertiseServerProperties: OnlineSubZomboy does not exist."));
	}
}