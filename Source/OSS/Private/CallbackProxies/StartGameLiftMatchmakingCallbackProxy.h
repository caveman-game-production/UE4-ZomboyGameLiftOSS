#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OSS/Public/OnlineSubsystemZomboyTypes.h"
#include "StartGameLiftMatchmakingCallbackProxy.generated.h"

class APlayerController;

UCLASS(MinimalAPI)
class UStartGameLiftMatchmakingCallbackProxy : public UOnlineBlueprintCallProxyBase
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
	static UStartGameLiftMatchmakingCallbackProxy* StartGameLiftMatchmaking(UObject* WorldContextObject);

	// UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	// End of UOnlineBlueprintCallProxyBase interface

private:

	void OnMatchmakingComplete(FName SessionName, bool bSuccessful);

	FOnMatchmakingCompleteDelegate OnMatchmakingCompleteDelegate;
	FDelegateHandle OnMatchmakingCompleteDelegateHandle;

	// The world context object in which this call is taking place
	UObject* WorldContextObject;

};
