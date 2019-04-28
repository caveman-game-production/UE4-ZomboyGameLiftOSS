#include "JoinGameLiftSessionsCallbackProxy.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "GameFramework/PlayerController.h"

//////////////////////////////////////////////////////////////////////////
// UGetFriendListCallbackProxy

UJoinGameLiftSessionsCallbackProxy::UJoinGameLiftSessionsCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UJoinGameLiftSessionsCallbackProxy* UJoinGameLiftSessionsCallbackProxy::JoinGameLiftSessions(UObject* WorldContextObject, const FGameLiftSessionResult& InSearchResult)
{
	UJoinGameLiftSessionsCallbackProxy* Proxy = NewObject<UJoinGameLiftSessionsCallbackProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->SearchResult = InSearchResult;

	return Proxy;
}

void UJoinGameLiftSessionsCallbackProxy::Activate()
{
	OnJoinGameLiftSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UJoinGameLiftSessionsCallbackProxy::OnJoinGameLiftSessionComplete);
	
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			OnJoinGameLiftSessiondCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinGameLiftSessionCompleteDelegate);

			if (!Sessions->JoinSession(0, GameSessionName, SearchResult.SearchResult))
			{
				Sessions->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinGameLiftSessiondCompleteDelegateHandle);
				OnFailure.Broadcast();
			}
		}
	}
}


void UJoinGameLiftSessionsCallbackProxy::OnJoinGameLiftSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Type::Success)
	{
		OnFailure.Broadcast();
		return;
	}

	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinGameLiftSessiondCompleteDelegateHandle);
			OnSuccess.Broadcast();
			return;
		}
	}

	OnFailure.Broadcast();
}
