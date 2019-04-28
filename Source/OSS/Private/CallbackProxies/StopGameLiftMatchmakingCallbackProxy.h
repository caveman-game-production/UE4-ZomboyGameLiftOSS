#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OSS/Public/OnlineSubsystemZomboyTypes.h"
#include "StopGameLiftMatchmakingCallbackProxy.generated.h"

class APlayerController;

UCLASS(MinimalAPI)
class UStopGameLiftMatchmakingCallbackProxy : public UOnlineBlueprintCallProxyBase
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
	static UStopGameLiftMatchmakingCallbackProxy* StopGameLiftMatchmaking(UObject* WorldContextObject);

	// UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	// End of UOnlineBlueprintCallProxyBase interface

private:

	void OnCancelMatchmakingComplete(FName SessionName, bool bSuccessful);

	FOnCancelMatchmakingCompleteDelegate OnCancelMatchmakingCompleteDelegate;
	FDelegateHandle OnCancelMatchmakingCompleteDelegateHandle;

	// The world context object in which this call is taking place
	UObject* WorldContextObject;

};
