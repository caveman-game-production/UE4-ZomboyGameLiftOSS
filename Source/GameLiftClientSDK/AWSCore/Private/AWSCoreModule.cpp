#include "AWSCoreModulePrivatePCH.h"
#include "AWSCoreModule.h"
#include "GameLiftClientSDK/GameLiftClientSDK/Public/GameLiftClientGlobals.h"
#include "IPluginManager.h"

#define LOCTEXT_NAMESPACE "FAWSCoreModule"

void* FAWSCoreModule::AWSCCommonLibraryHandle = nullptr;
void* FAWSCoreModule::AWSChecksumLibraryHandle = nullptr;
void* FAWSCoreModule::AWSCEventStreamLibraryHandle = nullptr;
void* FAWSCoreModule::AWSCoreLibraryHandle = nullptr;

void FAWSCoreModule::StartupModule()
{
#if PLATFORM_WINDOWS && PLATFORM_64BITS
	LOG_NORMAL("Starting AWSCore Module...");

	const FString BaseDir = IPluginManager::Get().FindPlugin("OnlineSubsystemZomboy")->GetBaseDir();
	LOG_NORMAL(FString::Printf(TEXT("Base directory is %s"), *BaseDir));

	const FString ThirdPartyDir = FPaths::Combine(*BaseDir, TEXT("ThirdParty"), TEXT("GameLiftClientSDK"), TEXT("Win64"));
	LOG_NORMAL(FString::Printf(TEXT("ThirdParty directory is %s"), *ThirdPartyDir));

	static const FString CCommonDLLName = "aws-c-common";
	const bool bCCommonDependencyLoaded = LoadDependency(ThirdPartyDir, CCommonDLLName, AWSCCommonLibraryHandle);
	if (bCCommonDependencyLoaded == false)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Name"), FText::FromString(CCommonDLLName));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("LoadDependencyError", "Failed to load {Name}. Plugin will not be functional"), Arguments));
		FreeDependency(AWSCCommonLibraryHandle);
		return;
	}

	static const FString ChecksumDLLName = "aws-checksums";
	const bool bChecksumDependencyLoaded = LoadDependency(ThirdPartyDir, ChecksumDLLName, AWSChecksumLibraryHandle);
	if (bChecksumDependencyLoaded == false)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Name"), FText::FromString(ChecksumDLLName));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("LoadDependencyError", "Failed to load {Name}. Plugin will not be functional"), Arguments));
		FreeDependency(AWSChecksumLibraryHandle);
		return;
	}

	static const FString CEventStreamDLLName = "aws-c-event-stream";
	const bool bCEventStreamDependencyLoaded = LoadDependency(ThirdPartyDir, CEventStreamDLLName, AWSCEventStreamLibraryHandle);
	if (bCEventStreamDependencyLoaded == false)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Name"), FText::FromString(CEventStreamDLLName));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("LoadDependencyError", "Failed to load {Name}. Plugin will not be functional"), Arguments));
		FreeDependency(AWSCEventStreamLibraryHandle);
		return;
	}

	static const FString CoreDLLName = "aws-cpp-sdk-core";
	const bool bCoreDependencyLoaded = LoadDependency(ThirdPartyDir, CoreDLLName, AWSCoreLibraryHandle);
	if (bCoreDependencyLoaded == false)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Name"), FText::FromString(CoreDLLName));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("LoadDependencyError", "Failed to load {Name}. Plugin will not be functional"), Arguments));
		FreeDependency(AWSCoreLibraryHandle);
		return;
	}
	
	Aws::InitAPI(options);
	LOG_NORMAL("Aws::InitAPI called.");
#endif
}

void FAWSCoreModule::ShutdownModule()
{
	Aws::ShutdownAPI(options);
	LOG_NORMAL("Aws::ShutdownAPI called.");
	FreeDependency(AWSCCommonLibraryHandle);
	FreeDependency(AWSChecksumLibraryHandle);
	FreeDependency(AWSCEventStreamLibraryHandle);
	FreeDependency(AWSCoreLibraryHandle);
	LOG_NORMAL("Shutting down AWSCore Module...");
}

bool FAWSCoreModule::LoadDependency(const FString& Dir, const FString& Name, void*& Handle)
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

void FAWSCoreModule::FreeDependency(void*& Handle)
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

IMPLEMENT_MODULE(FAWSCoreModule, AWSCore);
