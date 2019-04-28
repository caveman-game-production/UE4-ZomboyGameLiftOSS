#include "LoadPlayerIconImageCallbackProxy.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "GameFramework/PlayerController.h"

//////////////////////////////////////////////////////////////////////////
// UGetFriendListCallbackProxy

ULoadPlayerIconImageCallbackProxy::ULoadPlayerIconImageCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

ULoadPlayerIconImageCallbackProxy* ULoadPlayerIconImageCallbackProxy::LoadPlayerIconImage(UObject* WorldContextObject)
{
	ULoadPlayerIconImageCallbackProxy* Proxy = NewObject<ULoadPlayerIconImageCallbackProxy>();
	Proxy->WorldContextObject = WorldContextObject;

	return Proxy;
}

void ULoadPlayerIconImageCallbackProxy::Activate()
{
	OnLoadPlayerIconCompleteDelegate = FOnLoadPlayerIconCompleteDelegate::CreateUObject(this, &ULoadPlayerIconImageCallbackProxy::OnLoadingIconImageComplete);

	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		FOnlineIdentityZomboyPtr IdentityInt = StaticCastSharedPtr<FOnlineIdentityZomboy>(OnlineSub->GetIdentityInterface());
		if (IdentityInt.IsValid())
		{
			OnLoadPlayerIconCompleteDelegateHandle = IdentityInt->AddOnLoadPlayerIconCompleteDelegate_Handle(0, OnLoadPlayerIconCompleteDelegate);
			IdentityInt->LoadPlayerAvatarIcon(0);
			return;
		}
	}

	OnFailure.Broadcast(FZomboyOnlinePlayerIconInfo());
}

void ULoadPlayerIconImageCallbackProxy::OnLoadingIconImageComplete(int32 LocalPlayerNum, bool bWasSuccessful, const FZomboyOnlinePlayerIconInfo& IconInfo)
{
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		FOnlineIdentityZomboyPtr IdentityInt = StaticCastSharedPtr<FOnlineIdentityZomboy>(OnlineSub->GetIdentityInterface());
		if (IdentityInt.IsValid())
		{
			IdentityInt->ClearOnLoadPlayerIconCompleteDelegate_Handle(0, OnLoadPlayerIconCompleteDelegateHandle);
		}
	}


	if (bWasSuccessful)
	{
		OnSuccess.Broadcast(IconInfo);
	}
	else
	{
		OnFailure.Broadcast(FZomboyOnlinePlayerIconInfo());
	}
}
