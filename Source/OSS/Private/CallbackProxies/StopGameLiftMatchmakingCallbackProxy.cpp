#include "StopGameLiftMatchmakingCallbackProxy.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "GameFramework/PlayerController.h"

//////////////////////////////////////////////////////////////////////////
// UGetFriendListCallbackProxy

UStopGameLiftMatchmakingCallbackProxy::UStopGameLiftMatchmakingCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UStopGameLiftMatchmakingCallbackProxy* UStopGameLiftMatchmakingCallbackProxy::StopGameLiftMatchmaking(UObject* WorldContextObject)
{
	UStopGameLiftMatchmakingCallbackProxy* Proxy = NewObject<UStopGameLiftMatchmakingCallbackProxy>();
	Proxy->WorldContextObject = WorldContextObject;

	return Proxy;
}

void UStopGameLiftMatchmakingCallbackProxy::Activate()
{
	OnCancelMatchmakingCompleteDelegate = FOnCancelMatchmakingCompleteDelegate::CreateUObject(this, &UStopGameLiftMatchmakingCallbackProxy::OnCancelMatchmakingComplete);
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		// Get the Session Interface, so we can call the "CreateSession" function on it
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			OnCancelMatchmakingCompleteDelegateHandle = Sessions->AddOnCancelMatchmakingCompleteDelegate_Handle(OnCancelMatchmakingCompleteDelegate);
			Sessions->CancelMatchmaking(0, GameSessionName);
		}
	}
}

void UStopGameLiftMatchmakingCallbackProxy::OnCancelMatchmakingComplete(FName SessionName, bool bSuccessful)
{
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		// Get the Session Interface, so we can call the "CreateSession" function on it
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnMatchmakingCompleteDelegate_Handle(OnCancelMatchmakingCompleteDelegateHandle);
		}
	}


	if (!bSuccessful)
	{
		OnFailure.Broadcast();

		return;
	}

	OnSuccess.Broadcast();
}
