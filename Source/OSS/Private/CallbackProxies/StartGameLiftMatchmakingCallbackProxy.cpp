#include "StartGameLiftMatchmakingCallbackProxy.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "GameFramework/PlayerController.h"

//////////////////////////////////////////////////////////////////////////
// UGetFriendListCallbackProxy

UStartGameLiftMatchmakingCallbackProxy::UStartGameLiftMatchmakingCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UStartGameLiftMatchmakingCallbackProxy* UStartGameLiftMatchmakingCallbackProxy::StartGameLiftMatchmaking(UObject* WorldContextObject, const FZomboyGameliftMatchmakingSettingWrapper& MatchmakingSetting)
{
	UStartGameLiftMatchmakingCallbackProxy* Proxy = NewObject<UStartGameLiftMatchmakingCallbackProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->MatchmakingSetting = MatchmakingSetting;

	return Proxy;
}

void UStartGameLiftMatchmakingCallbackProxy::Activate()
{
	OnMatchmakingCompleteDelegate = FOnMatchmakingCompleteDelegate::CreateUObject(this, &UStartGameLiftMatchmakingCallbackProxy::OnMatchmakingComplete);
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		// Get the Session Interface, so we can call the "CreateSession" function on it
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			IOnlineIdentityPtr IdentityPtr = OnlineSub->GetIdentityInterface();
			if (IdentityPtr.IsValid())
			{
				TSharedPtr<const FUniqueNetId> PlayerNetId = IdentityPtr->GetUniquePlayerId(0);
				if (PlayerNetId.IsValid() && PlayerNetId->IsValid())
				{
					TArray<TSharedRef<const FUniqueNetId>> PlayerIds;
					PlayerIds.Add(PlayerNetId.ToSharedRef());

					OnMatchmakingCompleteDelegateHandle = Sessions->AddOnMatchmakingCompleteDelegate_Handle(OnMatchmakingCompleteDelegate);

					TSharedRef<FOnlineSessionSearch> SearchSettings((FOnlineSessionSearch*)(new FOnlineSessionSearchZomboy(MatchmakingSetting.OnlineSessionSearchSetting)));

					Sessions->StartMatchmaking(PlayerIds, GameSessionName, FOnlineSessionSettings(), SearchSettings);
				}
			}
		}
	}
}

void UStartGameLiftMatchmakingCallbackProxy::OnMatchmakingComplete(FName SessionName, bool bSuccessful)
{
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		// Get the Session Interface, so we can call the "CreateSession" function on it
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnMatchmakingCompleteDelegate_Handle(OnMatchmakingCompleteDelegateHandle);
		}
	}
	
	if (!bSuccessful)
	{
		OnFailure.Broadcast();

		return;
	}

	OnSuccess.Broadcast();
}
