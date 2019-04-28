// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "OnlineSubsystemZomboyModule.h"
#include "OnlineSubsystemModule.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemZomboy.h"


IMPLEMENT_MODULE(FOnlineSubsystemZomboyModule, OnlineSubsystemZomboy);

#if WITH_STEAM==1

bool FOnlineSubsystemZomboyModule::AreSteamDllsLoaded() const
{
	bool bLoadedClientDll = true;
	bool bLoadedServerDll = true;

#if LOADING_STEAM_LIBRARIES_DYNAMICALLY
	bLoadedClientDll = (SteamDLLHandle != NULL) ? true : false;
#if LOADING_STEAM_SERVER_LIBRARY_DYNAMICALLY
	bLoadedServerDll = IsRunningDedicatedServer() ? ((SteamServerDLLHandle != NULL) ? true : false) : true;
#endif //LOADING_STEAM_SERVER_LIBRARY_DYNAMICALLY
#endif // LOADING_STEAM_LIBRARIES_DYNAMICALLY

	return bLoadedClientDll && bLoadedServerDll;
}

static FString GetSteamModulePath()
{
#if PLATFORM_WINDOWS

#if PLATFORM_64BITS
	return FPaths::EngineDir() / STEAM_SDK_ROOT_PATH / STEAM_SDK_VER_PATH / TEXT("Win64/");
#else
	return FPaths::EngineDir() / STEAM_SDK_ROOT_PATH / STEAM_SDK_VER_PATH / TEXT("Win32/");
#endif	//PLATFORM_64BITS

#elif PLATFORM_LINUX

#if PLATFORM_64BITS
	return FPaths::EngineDir() / STEAM_SDK_ROOT_PATH / STEAM_SDK_VER_PATH / TEXT("x86_64-unknown-linux-gnu/");
#else
	return FPaths::EngineDir() / STEAM_SDK_ROOT_PATH / STEAM_SDK_VER_PATH / TEXT("i686-unknown-linux-gnu/");
#endif	//PLATFORM_64BITS

#else

	return FString();

#endif	//PLATFORM_WINDOWS
}

void FOnlineSubsystemZomboyModule::LoadSteamModules()
{
	UE_LOG_ONLINE(Display, TEXT("Loading Steam SDK %s"), STEAM_SDK_VER);

#if PLATFORM_WINDOWS

#if PLATFORM_64BITS
	FString Suffix("64");
#else
	FString Suffix;
#endif

	FString RootSteamPath = GetSteamModulePath();
	FPlatformProcess::PushDllDirectory(*RootSteamPath);
	SteamDLLHandle = FPlatformProcess::GetDllHandle(*(RootSteamPath + "steam_api" + Suffix + ".dll"));
	if (IsRunningDedicatedServer())
	{
		SteamServerDLLHandle = FPlatformProcess::GetDllHandle(*(RootSteamPath + "steamclient" + Suffix + ".dll"));
	}
	FPlatformProcess::PopDllDirectory(*RootSteamPath);
#elif PLATFORM_MAC
	SteamDLLHandle = FPlatformProcess::GetDllHandle(TEXT("libsteam_api.dylib"));
#elif PLATFORM_LINUX

#if LOADING_STEAM_LIBRARIES_DYNAMICALLY
	UE_LOG_ONLINE(Log, TEXT("Loading system libsteam_api.so."));
	SteamDLLHandle = FPlatformProcess::GetDllHandle(TEXT("libsteam_api.so"));
	if (SteamDLLHandle == nullptr)
	{
		// try bundled one
		UE_LOG_ONLINE(Warning, TEXT("Could not find system one, loading bundled libsteam_api.so."));
		FString RootSteamPath = GetSteamModulePath();
		SteamDLLHandle = FPlatformProcess::GetDllHandle(*(RootSteamPath + "libsteam_api.so"));
	}

	if (SteamDLLHandle)
	{
		UE_LOG_ONLINE(Display, TEXT("Loaded libsteam_api.so at %p"), SteamDLLHandle);
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Unable to load libsteam_api.so, Steam functionality will not work"));
	}
#else
	UE_LOG_ONLINE(Log, TEXT("libsteam_api.so is linked explicitly and should be already loaded."));
#endif // LOADING_STEAM_LIBRARIES_DYNAMICALLY

#endif	//PLATFORM_WINDOWS
}

void FOnlineSubsystemZomboyModule::UnloadSteamModules()
{
#if LOADING_STEAM_LIBRARIES_DYNAMICALLY
	if (SteamDLLHandle != NULL)
	{
		FPlatformProcess::FreeDllHandle(SteamDLLHandle);
		SteamDLLHandle = NULL;
	}

	if (SteamServerDLLHandle != NULL)
	{
		FPlatformProcess::FreeDllHandle(SteamServerDLLHandle);
		SteamServerDLLHandle = NULL;
	}
#endif	//LOADING_STEAM_LIBRARIES_DYNAMICALLY
}

#endif //WITH_STEAM

/**
* Class responsible for creating instance(s) of the subsystem
*/
class FOnlineFactoryZomboy : public IOnlineFactory
{
public:

	FOnlineFactoryZomboy() {}
	virtual ~FOnlineFactoryZomboy() {}

	virtual IOnlineSubsystemPtr CreateSubsystem(FName InstanceName)
	{
		FOnlineSubsystemZomboyPtr OnlineSub = MakeShared<FOnlineSubsystemZomboy, ESPMode::ThreadSafe>(InstanceName);
		if (OnlineSub->IsEnabled())
		{
			if (!OnlineSub->Init())
			{
				UE_LOG_ONLINE(Warning, TEXT("Zomboy API failed to initialize!"));
				OnlineSub->Shutdown();
				OnlineSub = NULL;
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("Zomboy API disabled!"));
			OnlineSub->Shutdown();
			OnlineSub = NULL;
		}

		return OnlineSub;
	}
};


void FOnlineSubsystemZomboyModule::StartupModule()
{
#if WITH_STEAM==1

	bool bSuccess = false;

	LoadSteamModules();
	if (AreSteamDllsLoaded())
	{
		ZomboyFactory = new FOnlineFactoryZomboy();

		// Create and register our singleton factory with the main online subsystem for easy access
		FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
		OSS.RegisterPlatformService(ZOMBOY_SUBSYSTEM, ZomboyFactory);

		bSuccess = true;
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Steam SDK %s libraries not present at %s or failed to load!"), STEAM_SDK_VER, *GetSteamModulePath());
	}

	if (!bSuccess)
	{
		UnloadSteamModules();
	}

#else
	ZomboyFactory = new FOnlineFactoryZomboy();

	// Create and register our singleton factory with the main online subsystem for easy access
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.RegisterPlatformService(ZOMBOY_SUBSYSTEM, ZomboyFactory);

#endif //WITH_STEAM
}

void FOnlineSubsystemZomboyModule::ShutdownModule()
{
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(ZOMBOY_SUBSYSTEM);
	
	delete ZomboyFactory;
	ZomboyFactory = NULL;

#if WITH_STEAM==1
	UnloadSteamModules();
#endif

}
