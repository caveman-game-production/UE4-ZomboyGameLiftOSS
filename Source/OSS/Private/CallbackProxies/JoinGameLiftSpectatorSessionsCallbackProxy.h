#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystemZomboyTypes.h"
#include "JoinGameLiftSpectatorSessionsCallbackProxy.generated.h"

class APlayerController;



UCLASS(MinimalAPI)
class UJoinGameLiftSpectatorSessionsCallbackProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

	// Called when the session was created successfully
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnSuccess;

	// Called when there was an error creating the session
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnFailure;

	// Creates a session with the default online subsystem
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly = "true", WorldContext="WorldContextObject"), Category = "Online|Session")
	static UJoinGameLiftSpectatorSessionsCallbackProxy* JoinGameLiftSpectatorSessions(UObject* WorldContextObject, const FGameLiftSessionResult& InSearchResult);

	// UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	// End of UOnlineBlueprintCallProxyBase interface

private:

	void OnJoinGameLiftSpectatorSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	FOnJoinSessionCompleteDelegate OnJoinGameLiftSessionCompleteDelegate;
	FDelegateHandle OnJoinGameLiftSessiondCompleteDelegateHandle;

	// The world context object in which this call is taking place
	UObject* WorldContextObject;
	FGameLiftSessionResult SearchResult;

};
