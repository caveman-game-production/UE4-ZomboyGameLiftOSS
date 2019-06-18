// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSubsystemZomboyTypes.h"
#include "OnlineSubsystemZomboy.h"
#include "OnlineSubsystemZomboyPrivate.h"
#include "Interfaces/OnlineIdentityInterface.h"


bool FUniqueNetIdZomboyPlayer::IsValid() const
{
	bool bIsValid = false;
	if (UniqueNetId64 != 0)
	{
		bIsValid = true;
	}

	return bIsValid;
}


static void WriteRawToTextureDynamic_RenderThread(FTexture2DDynamicResource* TextureResource, const TArray<uint8>& RawData, bool bUseSRGB = true)
{
#if !UE_SERVER

	check(IsInRenderingThread());

	FTexture2DRHIParamRef TextureRHI = TextureResource->GetTexture2DRHI();

	int32 Width = TextureRHI->GetSizeX();
	int32 Height = TextureRHI->GetSizeY();

	uint32 DestStride = 0;
	uint8* DestData = reinterpret_cast<uint8*>(RHILockTexture2D(TextureRHI, 0, RLM_WriteOnly, DestStride, false, false));

	for (int32 y = 0; y < Height; y++)
	{
		uint8* DestPtr = &DestData[(Height - 1 - y) * DestStride];

		const FColor* SrcPtr = &((FColor*)(RawData.GetData()))[(Height - 1 - y) * Width];
		for (int32 x = 0; x < Width; x++)
		{
			*DestPtr++ = SrcPtr->B;
			*DestPtr++ = SrcPtr->G;
			*DestPtr++ = SrcPtr->R;
			*DestPtr++ = SrcPtr->A;
			SrcPtr++;
		}
	}

	RHIUnlockTexture2D(TextureRHI, 0, false, false);

#endif
}

UZomboyOSSFunctionLibrary::UZomboyOSSFunctionLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FString UZomboyOSSFunctionLibrary::GetPlayerOnlineNickName()
{
	FOnlineSubsystemZomboy* OnlineSubZomboy = StaticCast<FOnlineSubsystemZomboy*>(IOnlineSubsystem::Get(FName("Zomboy")));
	if (OnlineSubZomboy)
	{
		auto IdentityPtr = OnlineSubZomboy->GetIdentityInterface();
		return IdentityPtr->GetPlayerNickname(0);
	}

	return TEXT("");
}

bool UZomboyOSSFunctionLibrary::IsValid(const FZomboyOnlinePlayerIconInfo& PlayerIconInfo)
{
	return PlayerIconInfo.IsValid();
}

UTexture2DDynamic* UZomboyOSSFunctionLibrary::GetPlayerIconTexture2DDynamic(const FZomboyOnlinePlayerIconInfo& PlayerIconInfo)
{
#if !UE_SERVER

	if (!PlayerIconInfo.IsValid())
	{
		return NULL;
	}

	UTexture2DDynamic* Avatar = UTexture2DDynamic::Create(PlayerIconInfo.Width, PlayerIconInfo.Height);

	Avatar->SRGB = true;
	Avatar->UpdateResource();

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FWriteRawDataToTextureDynamic,
		FTexture2DDynamicResource*, TextureResource, static_cast<FTexture2DDynamicResource*>(Avatar->Resource),
		TArray<uint8>, RawData, PlayerIconInfo.ImageData,
		{
			WriteRawToTextureDynamic_RenderThread(TextureResource, RawData);
		});

	return Avatar;
#endif
	return NULL;
}

EGameLiftRegion UZomboyOSSFunctionLibrary::GetGameLiftClientRegion()
{
	FOnlineSubsystemZomboy* OnlineSubZomboy = StaticCast<FOnlineSubsystemZomboy*>(IOnlineSubsystem::Get(FName("Zomboy")));
	if (OnlineSubZomboy)
	{
		return OnlineSubZomboy->GetGameLiftClient()->GetCurrentRegion();
	}

	return EGameLiftRegion::EGameLiftRegion_None;
}

void UZomboyOSSFunctionLibrary::SetGameLiftClientRegion(EGameLiftRegion Region)
{
	FOnlineSubsystemZomboy* OnlineSubZomboy = StaticCast<FOnlineSubsystemZomboy*>(IOnlineSubsystem::Get(FName("Zomboy")));
	if (OnlineSubZomboy)
	{
		OnlineSubZomboy->SetGameLiftClientRegion(Region);
	}
}