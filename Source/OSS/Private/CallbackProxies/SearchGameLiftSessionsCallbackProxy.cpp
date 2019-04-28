#include "SearchGameLiftSessionsCallbackProxy.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "GameFramework/PlayerController.h"

//////////////////////////////////////////////////////////////////////////
// UGetFriendListCallbackProxy

USearchGameLiftSessionsCallbackProxy::USearchGameLiftSessionsCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

USearchGameLiftSessionsCallbackProxy* USearchGameLiftSessionsCallbackProxy::SearchGameLiftSessions(UObject* WorldContextObject)
{
	USearchGameLiftSessionsCallbackProxy* Proxy = NewObject<USearchGameLiftSessionsCallbackProxy>();
	Proxy->WorldContextObject = WorldContextObject;


	return Proxy;
}

void USearchGameLiftSessionsCallbackProxy::Activate()
{

	Results.Empty();

	OnFindGameLiftSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &USearchGameLiftSessionsCallbackProxy::OnSearchGameLiftSessionsComplete);

	if (!SearchSessions())
	{
		TArray<FGameLiftSessionResult> EmptyResults;
		OnFailure.Broadcast(EmptyResults);
	}
}

bool USearchGameLiftSessionsCallbackProxy::SearchSessions()
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);

	if (World && World->GetGameInstance())
	{
		if (OnlineSub )
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid())
			{
				SearchSettings = MakeShareable(new FOnlineSessionSearch());
				SearchSettings->QuerySettings.Set(SEARCH_PRESENCE, false, EOnlineComparisonOp::Equals);
				TSharedRef<FOnlineSessionSearch> SearchSettingsRef = SearchSettings.ToSharedRef();

				OnFindGameLiftSessionsdCompleteDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(OnFindGameLiftSessionsCompleteDelegate);
				Sessions->FindSessions(0, SearchSettingsRef);

				return true;
			}
		}
	}

	return false;
}

void USearchGameLiftSessionsCallbackProxy::OnSearchGameLiftSessionsComplete(bool bSuccessful)
{	
	if (!bSuccessful)
	{
		TArray<FGameLiftSessionResult> EmptyResults;
		OnFailure.Broadcast(EmptyResults);

		return;
	}

	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(OnFindGameLiftSessionsdCompleteDelegateHandle);

			//UE_LOG(LogOnlineGame, Verbose, TEXT("Num Search Results: %d"), SearchSettings->SearchResults.Num());

			for (int32 SearchIdx = 0; SearchIdx < SearchSettings->SearchResults.Num(); SearchIdx++)
			{
				const FOnlineSessionSearchResult& SearchResult = SearchSettings->SearchResults[SearchIdx];

				/*FString MapName;
				SearchResult.Session.SessionSettings.Get(FName(TEXT("k_map")), MapName);*/

				int PlayerCapacity = SearchResult.Session.SessionSettings.NumPublicConnections;
				int PlayerCount = PlayerCapacity - SearchResult.Session.NumOpenPublicConnections;

				FGameLiftSessionResult GameLiftSearchResult;
				GameLiftSearchResult.SearchResult = SearchResult;
				GameLiftSearchResult.PlayerCapacity = PlayerCapacity;
				GameLiftSearchResult.PlayerCount = PlayerCount;
				//GameLiftSearchResult.MapName = MapName;

				for (auto SessionSetting : SearchResult.Session.SessionSettings.Settings)
				{
					if (SessionSetting.Key != NAME_None)
					{
						const auto& Data = SessionSetting.Value.Data;
						const auto AdvertisementType = SessionSetting.Value.AdvertisementType;

						//!!! We only accept string value
						if (Data.GetType() == EOnlineKeyValuePairDataType::String)
						{
							if (AdvertisementType == EOnlineDataAdvertisementType::ViaOnlineService)
							{
								GameLiftSearchResult.DynamicProperties.Add(SessionSetting.Key, Data.ToString());
							}
							else if (AdvertisementType == EOnlineDataAdvertisementType::ViaPingOnly)
							{
								GameLiftSearchResult.StaticProperties.Add(SessionSetting.Key, Data.ToString());
							}

						}
					}
				}

				TSharedPtr<FOnlineSessionInfoZomboy> ZomboySessionInfo = StaticCastSharedPtr<FOnlineSessionInfoZomboy>(SearchResult.Session.SessionInfo);
				GameLiftSearchResult.Region = ZomboySessionInfo->SessionRegion;
				
				Results.Add(GameLiftSearchResult);
			}

			
			OnSuccess.Broadcast(Results);
			return;
		}
	}

	
	TArray<FGameLiftSessionResult> EmptyResults;
	OnFailure.Broadcast(EmptyResults);
}
