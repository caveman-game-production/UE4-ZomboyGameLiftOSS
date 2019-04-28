// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "OnlineAsyncTaskManagerZomboy.h"
#include "OnlineSubsystemZomboy.h"


bool FOnlineAsyncTaskManagerZomboy::Init()
{
	if (FOnlineAsyncTaskManager::Init())
	{

#if WITH_OCULUS==1

	MessageTaskManager = MakeUnique<FOnlineMessageTaskManagerOculus>();
	check(MessageTaskManager);

#endif

		return true;
	}

	return false;
}

void FOnlineAsyncTaskManagerZomboy::OnlineTick()
{
	check(ZomboySubsystem);
	check(FPlatformTLS::GetCurrentThreadId() == OnlineThreadId || !FPlatformProcess::SupportsMultithreading());

#if WITH_STEAM
	if (ZomboySubsystem->IsOnlinePlatformAvailable())
	{
		SteamAPI_RunCallbacks();
	}
#elif WITH_OCULUS

	if (MessageTaskManager.IsValid())
	{
		if (!MessageTaskManager->Tick(0.011f))
		{
			UE_LOG_ONLINE(Error, TEXT("An error occured when processing the message queue"));
		}
	}

#endif
}

#if WITH_OCULUS

void FOnlineAsyncTaskManagerZomboy::AddRequestDelegate(ovrRequest RequestId, FOculusMessageOnCompleteDelegate&& Delegate) const
{
	check(MessageTaskManager);
	MessageTaskManager->AddRequestDelegate(RequestId, MoveTemp(Delegate));
}

FOculusMulticastMessageOnCompleteDelegate& FOnlineAsyncTaskManagerZomboy::GetNotifDelegate(ovrMessageType MessageType) const
{
	check(MessageTaskManager);
	return MessageTaskManager->GetNotifDelegate(MessageType);
}

void FOnlineAsyncTaskManagerZomboy::RemoveNotifDelegate(ovrMessageType MessageType, const FDelegateHandle& Delegate) const
{
	check(MessageTaskManager);
	return MessageTaskManager->RemoveNotifDelegate(MessageType, Delegate);
}


#endif

#if WITH_GAMELIFT_SERVER

FString FOnlineAsyncTaskGameLiftServerBackfillStart::ToString() const
{
	return FString::Printf(TEXT("FOnlineAsyncTaskGameLiftServerBackfillStart bWasSuccessful: %d"), bWasSuccessful);
 }

void FOnlineAsyncTaskGameLiftServerBackfillStart::TaskThreadInit()
{
	if (GameLiftServer.IsValid())
	{
		bool bDescribePlayersessionsSuccess = false;
		int ActivePlayerCount = MaxPlayers;
		{
			auto DescribePlayersessionsOutcome = GameLiftServer->DescribePlayerSessions(SessionId);
			if (DescribePlayersessionsOutcome.IsSuccess())
			{
				ActivePlayerCount = 0;
				bDescribePlayersessionsSuccess = true;
				int PlayerCount = 0;
				const Aws::GameLift::Server::Model::PlayerSession* PlayerSessionsPtr = DescribePlayersessionsOutcome.GetResult().GetPlayerSessions(PlayerCount);
				if (PlayerCount > 0)
				{
					for (int i = 0; i < PlayerCount; ++i)
					{
						const Aws::GameLift::Server::Model::PlayerSession PlayerSession = PlayerSessionsPtr[i];
						if (PlayerSession.GetStatus() == Aws::GameLift::Server::Model::PlayerSessionStatus::ACTIVE
							|| PlayerSession.GetStatus() == Aws::GameLift::Server::Model::PlayerSessionStatus::RESERVED)
						{
							ActivePlayerCount++;
						}
					}
				}
			}
		}

		UE_LOG(LogTemp, Log, TEXT("FOnlineAsyncTaskGameLiftServerBackfillStart Active Player Count: %d"), ActivePlayerCount);

		if (bDescribePlayersessionsSuccess)
		{
			if (ActivePlayerCount < MaxPlayers)
			{
				FStartMatchBackfillRequest Request(TicketId, SessionId, MatchmakingConfigArn, MatchmakingPlayers);

				FGameLiftStringOutcome Outcome = GameLiftServer->StartMatchBackfill(Request);
				if (Outcome.IsSuccess())
				{
					FinishTaskThread(true);
					return;
				}
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("FOnlineAsyncTaskGameLiftServerBackfillStart failed since reached player count limit."));
			}
		}
	}

	FinishTaskThread(false);
}

FString FOnlineAsyncTaskGameLiftServerBackfillStop::ToString() const
{
	return FString::Printf(TEXT("FOnlineAsyncTaskGameLiftServerBackfillStop bWasSuccessful: %d"), bWasSuccessful);
}

void FOnlineAsyncTaskGameLiftServerBackfillStop::TaskThreadInit()
{
	if (GameLiftServer.IsValid())
	{
		FStopMatchBackfillRequest Request(TicketId, SessionId, MatchmakingConfigArn);
		FGameLiftGenericOutcome StopBackFillOutcome = GameLiftServer->StopMatchBackfill(Request);
		if (StopBackFillOutcome.IsSuccess())
		{
			FinishTaskThread(true);
			return;
		}
	}

	FinishTaskThread(false);
}

FString FOnlineAsyncTaskGameLiftServerRemovePlayerSession::ToString() const
{
	return FString::Printf(TEXT("FOnlineAsyncTaskGameLiftServerRemovePlayerSession bWasSuccessful: %d"), bWasSuccessful);
}

void FOnlineAsyncTaskGameLiftServerRemovePlayerSession::TaskThreadInit()
{
	if (GameLiftServer.IsValid())
	{
		FGameLiftGenericOutcome Outcome = GameLiftServer->RemovePlayerSession(PlayerSessionId);
		if (Outcome.IsSuccess())
		{
			FinishTaskThread(true);
			return;
		}
	}

	FinishTaskThread(false);
}

void FOnlineAsyncTaskManagerZomboyServer::AddBackfillStartRequest(const FGameLiftServerPtr& GameLiftServer, const FString& SessionId, const FString& MatchmakingConfigArn, const TArray<FPlayer>& MatchmakingPlayers, int InMaxPlayers)
{
	if (CurrentBackfillTicketId.IsEmpty())
	{
		//add only a start backfill task 
		FGuid NewMatchmakingTicketGUID = FGuid::NewGuid();
		FString NewMatchmakingTicket = NewMatchmakingTicketGUID.ToString();
		//Add backfill task here
		FOnlineAsyncTaskGameLiftServerBackfillStart* BackfillStartTask = new FOnlineAsyncTaskGameLiftServerBackfillStart(ZomboySubsystem, GameLiftServer, NewMatchmakingTicket, SessionId, MatchmakingConfigArn, MatchmakingPlayers, InMaxPlayers);
		{
			FScopeLock LockInBackfillTaskQueue(&InBackfillTaskQueueLock);
			InBackfillTaskQueue.Add(BackfillStartTask);
		}
		CurrentBackfillTicketId = NewMatchmakingTicket;

		UE_LOG(LogTemp, Log, TEXT("AddBackfillStartRequest: Add fresh task."));

		return;
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("AddBackfillStartRequest: cancel then start new task."));


		//first add a backfill stop task
		FOnlineAsyncTaskGameLiftServerBackfillStop* BackfillStopTask = new FOnlineAsyncTaskGameLiftServerBackfillStop(ZomboySubsystem, GameLiftServer, CurrentBackfillTicketId, SessionId, MatchmakingConfigArn);
		{
			FScopeLock LockInBackfillTaskQueue(&InBackfillTaskQueueLock);
			InBackfillTaskQueue.Add(BackfillStopTask);
		}

		FGuid NewMatchmakingTicketGUID = FGuid::NewGuid();
		FString NewMatchmakingTicket = NewMatchmakingTicketGUID.ToString();
		//second add a backfill start task
		FOnlineAsyncTaskGameLiftServerBackfillStart* BackfillStartTask = new FOnlineAsyncTaskGameLiftServerBackfillStart(ZomboySubsystem, GameLiftServer, NewMatchmakingTicket, SessionId, MatchmakingConfigArn, MatchmakingPlayers, InMaxPlayers);
		{
			FScopeLock LockInBackfillTaskQueue(&InBackfillTaskQueueLock);
			InBackfillTaskQueue.Add(BackfillStartTask);
		}
		CurrentBackfillTicketId = NewMatchmakingTicket;
		return;
	}
}

void FOnlineAsyncTaskManagerZomboyServer::AddBackfillStopRequest(const FGameLiftServerPtr& GameLiftServer, const FString& SessionId, const FString& MatchmakingConfigArn)
{
	if (CurrentBackfillTicketId.IsEmpty())
	{
		//do nothing here since there is already a canceling in progress or backfill have already been canceled
		UE_LOG(LogTemp, Log, TEXT("AddBackfillStopRequest: Do nothing since there's no ticket to cancel"))
	}
	else
	{
		//Add a StopBackfillTask here
		FOnlineAsyncTaskGameLiftServerBackfillStop* BackfillStopTask = new FOnlineAsyncTaskGameLiftServerBackfillStop(ZomboySubsystem, GameLiftServer, CurrentBackfillTicketId, SessionId, MatchmakingConfigArn);
		{
			FScopeLock LockInBackfillTaskQueue(&InBackfillTaskQueueLock);
			InBackfillTaskQueue.Add(BackfillStopTask);
		}
	}

	CurrentBackfillTicketId.Empty();
}

void FOnlineAsyncTaskManagerZomboyServer::AddRemovePlayerSessionRequest(const FGameLiftServerPtr& GameLiftServer, const FString& PlayerSessionId)
{
	FOnlineAsyncTaskGameLiftServerRemovePlayerSession* Task = new FOnlineAsyncTaskGameLiftServerRemovePlayerSession(ZomboySubsystem, GameLiftServer, PlayerSessionId);
	AddToInQueue(Task);
}

void FOnlineAsyncTaskManagerZomboyServer::GameServerTick()
{
	GameTick();

	//Process Backfill Requests
	check(IsInGameThread());
	
	int32 QueueSize = 0;
	bool bHasActiveBackfillTask = false;
	{
		{
			FScopeLock LockInBackfillTaskQueue(&InBackfillTaskQueueLock);
			QueueSize = InBackfillTaskQueue.Num();

			//Drop repetitive backfill requests
			if (QueueSize > 2)
			{
				bool bIsOdd = (QueueSize % 2) != 0;
				if (bIsOdd)
				{
					FOnlineAsyncTaskGameLiftServerBackfill* First = InBackfillTaskQueue[0];
					FOnlineAsyncTaskGameLiftServerBackfill* Last = InBackfillTaskQueue.Last();

					FOnlineAsyncTaskGameLiftServerBackfill* Replacement = First->Type == EServerBackfillRequestType::EServerBackfillRequestType_Cancel ? First : Last;
					for (FOnlineAsyncTaskGameLiftServerBackfill* Task : InBackfillTaskQueue)
					{
						if (Task != Replacement)
						{
							delete Task;
						}
					}

					InBackfillTaskQueue.Empty();
					InBackfillTaskQueue.Add(Replacement);

					QueueSize = 1;
				}
				else
				{
					FOnlineAsyncTaskGameLiftServerBackfill* First = InBackfillTaskQueue[0];
					FOnlineAsyncTaskGameLiftServerBackfill* Last = InBackfillTaskQueue.Last();
					FOnlineAsyncTaskGameLiftServerBackfill* Replacement = First->Type == EServerBackfillRequestType::EServerBackfillRequestType_Cancel ? First : Last;
					while (InBackfillTaskQueue.Num() > 1)
					{
						FOnlineAsyncTaskGameLiftServerBackfill* Task = InBackfillTaskQueue.Pop();
						if (Task != Replacement)
						{
							delete Task;
						}
					}

					InBackfillTaskQueue.Add(Replacement);

					QueueSize = 2;
				}
			}
		}

		{
			FScopeLock LockActiveBackfillTask(&ActiveBackfillTaskLock);
			if (ActiveBackfillTask != nullptr)
			{
				++QueueSize;
				bHasActiveBackfillTask = true;
				UE_LOG(LogTemp, Log, TEXT("Have ActiveBackfillTask......... %d"), (int)ActiveBackfillTask->Type);
			}
		}

		if (!bHasActiveBackfillTask && QueueSize > 0)
		{

			UE_LOG(LogTemp, Log, TEXT("Have queued tasks........"));

			// Grab the current task from the queue
			FOnlineAsyncTaskGameLiftServerBackfill* Task = nullptr;
			{
				FScopeLock LockInBackfillTaskQueue(&InBackfillTaskQueueLock);
				Task = InBackfillTaskQueue[0];
				InBackfillTaskQueue.RemoveAt(0);
			}

			// Initialize the task before giving it away to the online thread
			Task->Initialize();
			{
				FScopeLock LockActiveBackfillTask(&ActiveBackfillTaskLock);
				ActiveBackfillTask = Task;
			}

			// Wake up the online thread
			//WorkEvent->Trigger();
		}
	}

}

void FOnlineAsyncTaskManagerZomboyServer::Tick()
{
	FOnlineAsyncTaskManager::Tick();

	{
		//Process Backfill Requests
		FOnlineAsyncTaskGameLiftServerBackfill* Task = nullptr;
		{
			FScopeLock LockActiveBackfillTask(&ActiveBackfillTaskLock);
			Task = ActiveBackfillTask;
		}

		if (Task)
		{
			Task->Tick();

			if (Task->IsDone())
			{
				if (Task->WasSuccessful())
				{
					UE_LOG(LogOnline, Verbose, TEXT("Async task '%s' succeeded in %f seconds"),
						*Task->ToString(),
						Task->GetElapsedTime());
				}
				else
				{
					UE_LOG(LogOnline, Warning, TEXT("Async task '%s' failed in %f seconds"),
						*Task->ToString(),
						Task->GetElapsedTime());
				}

				// Task is done, add to the outgoing queue
				AddToOutQueue(Task);

				{
					FScopeLock LockActiveBackfillTask(&ActiveBackfillTaskLock);
					ActiveBackfillTask = nullptr;
				}
			}
		}
	}
}

#endif //WITH_GAMELIFT_SERVER
