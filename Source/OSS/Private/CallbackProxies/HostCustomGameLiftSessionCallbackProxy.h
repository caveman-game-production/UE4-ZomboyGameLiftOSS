#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystemZomboyTypes.h"
#include "HostCustomGameLiftSessionCallbackProxy.generated.h"

UCLASS(MinimalAPI)
class UHostCustomGameLiftSessionCallbackProxy : public UOnlineBlueprintCallProxyBase
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
	static UHostCustomGameLiftSessionCallbackProxy* HostCustomGameLiftSession(UObject* WorldContextObject, int MaxPlayers, const TMap<FName, FGameLiftGameSessionSetting>& GameProperties);

	// UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	// End of UOnlineBlueprintCallProxyBase interface

private:
	// Internal callback when session creation completes, calls StartSession
	UFUNCTION()
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	FOnCreateSessionCompleteDelegate OnCreateGameLiftSessionCompleteDelegate;
	FDelegateHandle OnCreateGameLiftSessionCompleteDelegateHandle;

	// The world context object in which this call is taking place
	UObject* WorldContextObject;

	int MaxPlayers = 0;

	TMap<FName, FGameLiftGameSessionSetting> Properties;
};
