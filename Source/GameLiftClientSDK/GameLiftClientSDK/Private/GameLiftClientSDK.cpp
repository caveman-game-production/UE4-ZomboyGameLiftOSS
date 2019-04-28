// Created by YetiTech Studios.

#include "GameLiftClientSDK.h"
#include "GameLiftClientGlobals.h"
#include "Core.h"
#include "ModuleManager.h"
#include "IPluginManager.h"

#define LOCTEXT_NAMESPACE "FGameLiftClientSDKModule"

namespace Aws
{
	namespace Http
	{

		const char* DATE_HEADER = "date";
		const char* AWS_DATE_HEADER = "X-Amz-Date";
		const char* AWS_SECURITY_TOKEN = "X-Amz-Security-Token";
		const char* ACCEPT_HEADER = "accept";
		const char* ACCEPT_CHAR_SET_HEADER = "accept-charset";
		const char* ACCEPT_ENCODING_HEADER = "accept-encoding";
		const char* AUTHORIZATION_HEADER = "authorization";
		const char* AWS_AUTHORIZATION_HEADER = "authorization";
		const char* COOKIE_HEADER = "cookie";
		const char* CONTENT_LENGTH_HEADER = "content-length";
		const char* CONTENT_TYPE_HEADER = "content-type";
		const char* USER_AGENT_HEADER = "user-agent";
		const char* VIA_HEADER = "via";
		const char* HOST_HEADER = "host";
		const char* AMZ_TARGET_HEADER = "x-amz-target";
		const char* X_AMZ_EXPIRES_HEADER = "X-Amz-Expires";
		const char* CONTENT_MD5_HEADER = "content-md5";
		const char* API_VERSION_HEADER = "x-amz-api-version";

	} // Http
} // Aws


void* FGameLiftClientSDKModule::GameLiftClientSDKLibraryHandle = nullptr;

void FGameLiftClientSDKModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
#if PLATFORM_WINDOWS && PLATFORM_64BITS && WITH_GAMELIFTCLIENTSDK
	LOG_NORMAL("Starting GameLiftClient Module...");
	// Get the base directory of this plugin
	const FString BaseDir = IPluginManager::Get().FindPlugin("OnlineSubsystemZomboy")->GetBaseDir();
	LOG_NORMAL(FString::Printf(TEXT("Base directory is %s"), *BaseDir));

	// Add on the relative location of the third party dll and load it
	const FString ThirdPartyDir = FPaths::Combine(*BaseDir, TEXT("ThirdParty"), TEXT("GameLiftClientSDK"), TEXT("Win64"));
	LOG_NORMAL(FString::Printf(TEXT("ThirdParty directory is %s"), *ThirdPartyDir));

	static const FString GameLiftDLLName = "aws-cpp-sdk-gamelift";
	const bool bDependencyLoaded = LoadDependency(ThirdPartyDir, GameLiftDLLName, GameLiftClientSDKLibraryHandle);
	if (!bDependencyLoaded)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Name"), FText::FromString(GameLiftDLLName));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("LoadDependencyError", "Failed to load {Name}."), Arguments));
		FreeDependency(GameLiftClientSDKLibraryHandle);
	}
#endif
}

void FGameLiftClientSDKModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.	
	FreeDependency(GameLiftClientSDKLibraryHandle);	
	LOG_NORMAL("Shutting down GameLift Module...");
}

bool FGameLiftClientSDKModule::LoadDependency(const FString& Dir, const FString& Name, void*& Handle)
{
	FString Lib = Name + TEXT(".") + FPlatformProcess::GetModuleExtension();
	FString Path = Dir.IsEmpty() ? *Lib : FPaths::Combine(*Dir, *Lib);

	Handle = FPlatformProcess::GetDllHandle(*Path);

	if (Handle == nullptr)
	{		
		LOG_ERROR(FString::Printf(TEXT("Dependency %s failed to load in directory %s"), *Lib, *Dir));
		return false;
	}

	LOG_NORMAL(FString::Printf(TEXT("Dependency %s successfully loaded from directory %s"), *Lib, *Dir));
	return true;
}

void FGameLiftClientSDKModule::FreeDependency(void*& Handle)
{
#if !PLATFORM_LINUX
	if (Handle != nullptr)
	{
		FPlatformProcess::FreeDllHandle(Handle);
		Handle = nullptr;
	}
#endif
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGameLiftClientSDKModule, GameLiftClientSDK)
