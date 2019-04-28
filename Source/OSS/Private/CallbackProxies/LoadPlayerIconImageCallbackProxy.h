#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OSS/Public/OnlineSubsystemZomboyTypes.h"
#include "OSS/Private/OnlineIdentityZomboy.h"
#include "Engine/Texture2DDynamic.h"
#include "LoadPlayerIconImageCallbackProxy.generated.h"

class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLoadPlayerIconImage, const FZomboyOnlinePlayerIconInfo&, IconInfo);

UCLASS(MinimalAPI)
class ULoadPlayerIconImageCallbackProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

	// Called when the session was created successfully
	UPROPERTY(BlueprintAssignable)
	FLoadPlayerIconImage OnSuccess;

	// Called when there was an error creating the session
	UPROPERTY(BlueprintAssignable)
	FLoadPlayerIconImage OnFailure;

	// Creates a session with the default online subsystem
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly = "true", WorldContext="WorldContextObject"), Category = "Online|Session")
	static ULoadPlayerIconImageCallbackProxy* LoadPlayerIconImage(UObject* WorldContextObject);

	// UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	// End of UOnlineBlueprintCallProxyBase interface

private:

	void OnLoadingIconImageComplete(int32 LocalPlayerNum, bool bWasSuccessful, const FZomboyOnlinePlayerIconInfo& IconInfo);

	FOnLoadPlayerIconCompleteDelegate OnLoadPlayerIconCompleteDelegate;
	FDelegateHandle OnLoadPlayerIconCompleteDelegateHandle;

	// The world context object in which this call is taking place
	UObject* WorldContextObject;

};
