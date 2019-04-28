#include "HostCustomGameLiftSessionCallbackProxy.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include <string>
#include <codecvt>
//////////////////////////////////////////////////////////////////////////
// UHostLobbySessionCallbackProxy

UHostCustomGameLiftSessionCallbackProxy::UHostCustomGameLiftSessionCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UHostCustomGameLiftSessionCallbackProxy* UHostCustomGameLiftSessionCallbackProxy::HostCustomGameLiftSession(UObject* WorldContextObject, int MaxPlayers, const TMap<FName, FGameLiftGameSessionSetting>& GameProperties)
{
	UHostCustomGameLiftSessionCallbackProxy* Proxy = NewObject<UHostCustomGameLiftSessionCallbackProxy>();
	Proxy->WorldContextObject = WorldContextObject;
	Proxy->MaxPlayers = MaxPlayers;
	Proxy->Properties = GameProperties;
	return Proxy;
}

void UHostCustomGameLiftSessionCallbackProxy::Activate()
{
	OnCreateGameLiftSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UHostCustomGameLiftSessionCallbackProxy::OnCreateSessionComplete);

	int MaxNumPlayers = 10;

	// Get the Online Subsystem to work with
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		// Get the Session Interface, so we can call the "CreateSession" function on it
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			TSharedPtr<FOnlineSessionSettings> HostSettings = MakeShareable(new FOnlineSessionSettings());
			HostSettings->bIsLANMatch = false;
			HostSettings->bUsesPresence = false;
			HostSettings->NumPublicConnections = MaxNumPlayers;
			HostSettings->NumPrivateConnections = 0;

			//HostSettings->Set(TEXT("k_map"), MapName, EOnlineDataAdvertisementType::ViaOnlineService);
			for (const auto& Property : Properties)
			{
				if (Property.Key == NAME_None || Property.Value.Value.IsEmpty()) continue;

				if (Property.Value.Type == EGameLiftGameSessionSettingType::EGameLiftGameSessionSettingType_Dynamic)
				{
					HostSettings->Set(Property.Key, Property.Value.Value, EOnlineDataAdvertisementType::ViaOnlineService);
				}
				else
				{
					HostSettings->Set(Property.Key, Property.Value.Value, EOnlineDataAdvertisementType::ViaPingOnly);
				}
			}
			
			OnCreateGameLiftSessionCompleteDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(OnCreateGameLiftSessionCompleteDelegate);

			if (!Sessions->CreateSession(0, GameSessionName, *HostSettings))
			{
				Sessions->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateGameLiftSessionCompleteDelegateHandle);
				OnFailure.Broadcast();
			}
		}
	}
}

void UHostCustomGameLiftSessionCallbackProxy::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		// Get the Session Interface, so we can call the "CreateSession" function on it
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateGameLiftSessionCompleteDelegateHandle);
		}
	}

	if (bWasSuccessful)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast();
	}
}

