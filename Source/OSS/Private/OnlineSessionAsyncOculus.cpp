#include "OnlineSessionAsyncOculus.h"
#include "SocketSubsystem.h"
#include "OnlineSessionInterfaceZomboy.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineIdentityZomboy.h"
#include "OnlineSubsystemZomboy.h"
#include <sstream>
#include <string>

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Interfaces/IHttpResponse.h"
#include "HttpModule.h"
#include "Engine/Texture2DDynamic.h"
#include "Runtime/RenderCore/Public/RenderingThread.h"


#if WITH_OCULUS

FString FOnlineAsyncTaskOculusConfirmLogin::ToString() const
{
	return FString::Printf(TEXT("FOnlineAsyncTaskOculusConfirmLogin bWasSuccessful: %d"), bWasSuccessful);
}

void FOnlineAsyncTaskOculusConfirmLogin::TaskThreadInit()
{
	if (Subsystem && Subsystem->GetAsyncTaskManager())
	{
		Subsystem->GetAsyncTaskManager()->AddRequestDelegate(
			ovr_User_GetLoggedInUser(),
			FOculusMessageOnCompleteDelegate::CreateRaw(this, &FOnlineAsyncTaskOculusConfirmLogin::OnLoginComplete, 0));
	}
	else
	{
		FinishTaskThread(false);
	}

}

void FOnlineAsyncTaskOculusConfirmLogin::OnLoginComplete(ovrMessageHandle Message, bool bIsError, int32 InLocalUserNum)
{
	MessageHandle = Message;
	LocalUserNum = InLocalUserNum;

	if (!bIsError)
	{
		OVRUser = ovr_Message_GetUser(MessageHandle);

		UserName = FString(ovr_User_GetOculusID(OVRUser));
		UserId = ovr_User_GetID(OVRUser);

	}

	FinishTaskThread(!bIsError);
}

void FOnlineAsyncTaskOculusConfirmLogin::TriggerDelegates()
{
	FString ErrorStr;// = FString(TEXT("FOnlineAsyncTaskSteamCreateLobby::TriggerDelegates, FOnlineIdentityZomboyPtr is not found"));

	FOnlineIdentityZomboyPtr IdentityInt = StaticCastSharedPtr<FOnlineIdentityZomboy>(Subsystem->GetIdentityInterface());
	if (IdentityInt.IsValid())
	{
		if (!bWasSuccessful)
		{
			auto Error = ovr_Message_GetError(MessageHandle);
			auto ErrorMessage = ovr_Error_GetMessage(Error);
			ErrorStr = FString(ErrorMessage);
		}
		else
		{
			uint64 oculus_userid_u64 = UserId;
			FUniqueNetIdZomboyPlayer CureantOVRID(oculus_userid_u64);
			
			TSharedPtr<const FUniqueNetId> NewUserId = IdentityInt->GetUniquePlayerId(LocalUserNum); 
			if ( !NewUserId.IsValid() || static_cast<const FUniqueNetIdZomboyPlayer>(*NewUserId.Get()) != CureantOVRID)
			{
				IdentityInt->AddUniquePlayerId(LocalUserNum, MakeShareable(new FUniqueNetIdZomboyPlayer(CureantOVRID)));
				NewUserId = IdentityInt->GetUniquePlayerId(LocalUserNum);
			}

			if (!NewUserId->IsValid())
			{
				ErrorStr = FString(TEXT("Unable to get a valid ID"));
			}
			else
			{
				TSharedRef<FUserOnlineAccountZomboy> UserAccountRef(new FUserOnlineAccountZomboy(NewUserId.ToSharedRef(), UserName));

				// update/add cached entry for user
				IdentityInt->AddUserAccount(static_cast<FUniqueNetIdZomboyPlayer>(*UserAccountRef->GetUserId()), UserAccountRef);

				IdentityInt->TriggerOnLoginCompleteDelegates(LocalUserNum, true, *UserAccountRef->GetUserId(), *ErrorStr);
				IdentityInt->TriggerOnLoginStatusChangedDelegates(LocalUserNum, ELoginStatus::NotLoggedIn, ELoginStatus::LoggedIn, *UserAccountRef->GetUserId());
				return;
			}
		}

		IdentityInt->TriggerOnLoginCompleteDelegates(LocalUserNum, false, FUniqueNetIdZomboyPlayer(), *ErrorStr);
	}
	else
	{
		IdentityInt->TriggerOnLoginCompleteDelegates(LocalUserNum, false, FUniqueNetIdZomboyPlayer(), *ErrorStr);
	}
}

FString FOnlineAsyncTaskOculusLoadPlayerImage::ToString() const
{
	return FString::Printf(TEXT("FOnlineAsyncTaskOculusLoadPlayerImage bWasSuccessful: %d"), bWasSuccessful);
}

void FOnlineAsyncTaskOculusLoadPlayerImage::TaskThreadInit()
{
	if (Subsystem && Subsystem->GetAsyncTaskManager())
	{
		Subsystem->GetAsyncTaskManager()->AddRequestDelegate(
			ovr_User_GetLoggedInUser(),
			FOculusMessageOnCompleteDelegate::CreateRaw(this, &FOnlineAsyncTaskOculusLoadPlayerImage::OnGetLoggedInPlayerComplete, 0));
	}
	else
	{
		FinishTaskThread(false);
	}
}

void FOnlineAsyncTaskOculusLoadPlayerImage::OnGetLoggedInPlayerComplete(ovrMessageHandle Message, bool bIsError, int32 InLocalUserNum)
{
	if (!bIsError)
	{
		MessageHandle = Message;
		ovrUserHandle OVRUser = ovr_Message_GetUser(MessageHandle);
		const char* ImageURLCString = ovr_User_GetImageUrl(OVRUser);

		FString ImageURL = FString(ImageURLCString);
		
		// Create the Http request and add to pending request list
		TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

		HttpRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineAsyncTaskOculusLoadPlayerImage::HandleImageRequest);
		HttpRequest->SetURL(ImageURL);
		HttpRequest->SetVerb(TEXT("GET"));
		HttpRequest->ProcessRequest();

		return;
	}
	else
	{
		FPlatformMisc::RequestExit(false);
	}

	FinishTaskThread(false);
}

static TArray<uint8> ResizeImageBilinear(const uint8* Buffer, int w, int h, int w2, int h2)
{
	TArray<uint8> temp;
	temp.AddUninitialized(w2 * h2 * 4);
	uint8* temp_buffer = temp.GetData();

	int x, y;
	int index = 0;
	float x_ratio = ((float)(w - 1)) / w2;
	float y_ratio = ((float)(h - 1)) / h2;
	float x_diff, y_diff, blue, green, red, alpha;
	int offset = 0;

	for (int i = 0; i < h2; i++)
	{
		for (int j = 0; j < w2; j++)
		{
			x = (int)(x_ratio * j);
			y = (int)(y_ratio * i);
			x_diff = (x_ratio * j) - x;
			y_diff = (y_ratio * i) - y;
			index = (y*w + x);

			int index_a = index;
			int index_b = (index + 1);
			int index_c = (index + w);
			int index_d = (index + w + 1);

			const FColor* a = &((FColor*)(Buffer))[index_a];
			const FColor* b = &((FColor*)(Buffer))[index_b];
			const FColor* c = &((FColor*)(Buffer))[index_c];
			const FColor* d = &((FColor*)(Buffer))[index_d];
			
			blue = a->B * (1 - x_diff) * (1 - y_diff) + b->B * (x_diff) * (1 - y_diff) + c->B * (y_diff)* (1 - x_diff) + d->B * (x_diff * y_diff);
			green = a->G * (1 - x_diff) * (1 - y_diff) + b->G * (x_diff) * (1 - y_diff) + c->G * (y_diff)* (1 - x_diff) + d->G * (x_diff * y_diff);
			red = a->R * (1 - x_diff) * (1 - y_diff) + b->R * (x_diff) * (1 - y_diff) + c->R * (y_diff)* (1 - x_diff) + d->R * (x_diff * y_diff);
			alpha = a->A * (1 - x_diff) * (1 - y_diff) + b->A * (x_diff) * (1 - y_diff) + c->A * (y_diff)* (1 - x_diff) + d->A * (x_diff * y_diff);

			*temp_buffer++ = blue;
			*temp_buffer++ = green;
			*temp_buffer++ = red;
			*temp_buffer++ = alpha;
		}
	}

	return temp;
}

void FOnlineAsyncTaskOculusLoadPlayerImage::HandleImageRequest(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
#if !UE_SERVER

	if (bSucceeded && HttpResponse.IsValid() && HttpResponse->GetContentLength() > 0)
	{
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> ImageWrappers[3] =
		{
			ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG),
			ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG),
			ImageWrapperModule.CreateImageWrapper(EImageFormat::BMP),
		};

		for (auto ImageWrapper : ImageWrappers)
		{
			if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(HttpResponse->GetContent().GetData(), HttpResponse->GetContentLength()))
			{
				const TArray<uint8>* RawData = NULL;
				if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData))
				{
					int32 Width = ImageWrapper->GetWidth();
					int32 Height = ImageWrapper->GetHeight();

					TArray<uint8> ResizedRawData = ResizeImageBilinear((*RawData).GetData(), Width, Height, 64, 64);

					PlayerIconInfo = FZomboyOnlinePlayerIconInfo(64, 64, ResizedRawData);
					if (PlayerIconInfo.IsValid())
					{
						FinishTaskThread(true);
						return;
					}
				}
			}
		}
	}

	FinishTaskThread(false);

#endif
}


void FOnlineAsyncTaskOculusLoadPlayerImage::TriggerDelegates() 
{
	FOnlineIdentityZomboyPtr IdentityInt = StaticCastSharedPtr<FOnlineIdentityZomboy>(Subsystem->GetIdentityInterface());
	if (IdentityInt.IsValid())
	{
		IdentityInt->TriggerOnLoadPlayerIconCompleteDelegates(0, bWasSuccessful, PlayerIconInfo);
	}
}


#endif
