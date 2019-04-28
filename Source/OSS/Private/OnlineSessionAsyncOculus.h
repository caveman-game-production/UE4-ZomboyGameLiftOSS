#pragma once

#include "CoreMinimal.h"
#include "UObject/CoreOnline.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineAsyncTaskManager.h"
#include "OnlineAsyncTaskManagerZomboy.h"
#include "Interfaces/IHttpRequest.h"
#include "OnlineSubsystemZomboyTypes.h"
#include "OnlineSubsystemZomboyPackage.h"

#if WITH_OCULUS

class FOnlineAsyncTaskOculusConfirmLogin : public FOnlineAsyncTaskOculus
{
private:
	/** Hidden on purpose */
	FOnlineAsyncTaskOculusConfirmLogin() :
		LocalUserNum(0),
		OVRUser(NULL),
		UserName(FString()),
		UserId(0)
	{

	}

public:
	FOnlineAsyncTaskOculusConfirmLogin(class FOnlineSubsystemZomboy* InSubsystem) :
		FOnlineAsyncTaskOculus(InSubsystem),
		LocalUserNum(0),
		OVRUser(NULL),
		UserName(FString()),
		UserId(0)
	{

	}

protected:

	ovrMessageHandle MessageHandle;
	int32 LocalUserNum;
	ovrUserHandle OVRUser;
	FString UserName;
	ovrID UserId;

protected:

	virtual FString ToString() const override;
	
	virtual void TaskThreadInit() override;

	void OnLoginComplete(ovrMessageHandle Message, bool bIsError, int32 LocalUserNum);

	virtual void TriggerDelegates() override;
	
};

class FOnlineAsyncTaskOculusLoadPlayerImage : public FOnlineAsyncTaskOculus
{
private:
	/** Hidden on purpose */
	FOnlineAsyncTaskOculusLoadPlayerImage() :
		PlayerIconInfo(FZomboyOnlinePlayerIconInfo())
	{

	}

public:
	FOnlineAsyncTaskOculusLoadPlayerImage(class FOnlineSubsystemZomboy* InSubsystem) :
		FOnlineAsyncTaskOculus(InSubsystem),
		PlayerIconInfo(FZomboyOnlinePlayerIconInfo())
	{

	}

protected:
	ovrMessageHandle MessageHandle;
	FZomboyOnlinePlayerIconInfo PlayerIconInfo;

protected:

	virtual FString ToString() const override;

	virtual void TaskThreadInit() override;

	void OnGetLoggedInPlayerComplete(ovrMessageHandle Message, bool bIsError, int32 InLocalUserNum);

	void HandleImageRequest(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	virtual void TriggerDelegates() override;

};

#endif
