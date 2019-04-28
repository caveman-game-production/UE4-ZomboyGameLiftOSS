#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystemZomboyTypes.h"
#include "SearchGameLiftSessionsCallbackProxy.generated.h"

class APlayerController;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSearchGameLiftSessions, const TArray<FGameLiftSessionResult>&, Results);

UCLASS(MinimalAPI)
class USearchGameLiftSessionsCallbackProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

	// Called when the session was created successfully
	UPROPERTY(BlueprintAssignable)
	FSearchGameLiftSessions OnSuccess;

	// Called when there was an error creating the session
	UPROPERTY(BlueprintAssignable)
	FSearchGameLiftSessions OnFailure;

	// Creates a session with the default online subsystem
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly = "true", WorldContext="WorldContextObject"), Category = "Online|Session")
	static USearchGameLiftSessionsCallbackProxy* SearchGameLiftSessions(UObject* WorldContextObject);

	// UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	// End of UOnlineBlueprintCallProxyBase interface

private:

	UFUNCTION()
	bool SearchSessions();
	UFUNCTION()
	void OnSearchGameLiftSessionsComplete(bool bSuccessful);
	FOnFindSessionsCompleteDelegate OnFindGameLiftSessionsCompleteDelegate;
	FDelegateHandle OnFindGameLiftSessionsdCompleteDelegateHandle;

	// The world context object in which this call is taking place
	UObject* WorldContextObject;

	TSharedPtr<class FOnlineSessionSearch> SearchSettings;
	TArray<FGameLiftSessionResult> Results;

	
};
