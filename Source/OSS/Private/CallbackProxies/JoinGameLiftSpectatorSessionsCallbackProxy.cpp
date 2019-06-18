#include "JoinGameLiftSpectatorSessionsCallbackProxy.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "GameFramework/PlayerController.h"

//////////////////////////////////////////////////////////////////////////
// UGetFriendListCallbackProxy

UJoinGameLiftSpectatorSessionsCallbackProxy::UJoinGameLiftSpectatorSessionsCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UJoinGameLiftSpectatorSessionsCallbackProxy* UJoinGameLiftSpectatorSessionsCallbackProxy::JoinGameLiftSpectatorSessions(UObject* WorldContextObject, const FGameLiftSessionResult& InSearchResult)
{
	UJoinGameLiftSpectatorSessionsCallbackProxy* Proxy = NewObject<UJoinGameLiftSpectatorSessionsCallbackProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->SearchResult = InSearchResult;

	return Proxy;
}

void UJoinGameLiftSpectatorSessionsCallbackProxy::Activate()
{
	OnJoinGameLiftSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UJoinGameLiftSpectatorSessionsCallbackProxy::OnJoinGameLiftSpectatorSessionComplete);
	
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			OnJoinGameLiftSessiondCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinGameLiftSessionCompleteDelegate);

			if (!Sessions->JoinSession(0, SpectatorSessionName, SearchResult.SearchResult))
			{
				Sessions->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinGameLiftSessiondCompleteDelegateHandle);
				OnFailure.Broadcast();
			}
		}
	}
}


void UJoinGameLiftSpectatorSessionsCallbackProxy::OnJoinGameLiftSpectatorSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
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
