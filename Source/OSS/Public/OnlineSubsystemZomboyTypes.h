// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemTypes.h"
#include "IPAddress.h"
#include "OnlineSessionSettings.h"
#include "Engine/Texture2DDynamic.h"
#include "Runtime/RenderCore/Public/RenderingThread.h"
#include <string>
#include <codecvt>

#include "OnlineSubsystemZomboyTypes.generated.h"


class FOnlineSubsystemClass;

#define GAMELIFT_URL_PLAYER_SESSION_ID TEXT("psid")
#define GAMELIFT_URL_PLAYER_REGION_KEY TEXT("region")
#define GAMELIFT_URL_PLAYER_JOINTYPE_KEY TEXT("jointype")
#define GAMELIFT_PROPERTY_MAP_KEY "k_map"

/**
 * Unique net id wrapper for a string
 */
class FUniqueNetIdZomboyPlayer : public FUniqueNetId
{
public:
	/** Holds the net id for a player */
	//FString UniqueNetIdStr;
	uint64 UniqueNetId64;

	// Define these to increase visibility to public (from parent's protected)
	FUniqueNetIdZomboyPlayer() = default;

	FUniqueNetIdZomboyPlayer(FUniqueNetIdZomboyPlayer&&) = default;
	FUniqueNetIdZomboyPlayer(const FUniqueNetIdZomboyPlayer&) = default;
	FUniqueNetIdZomboyPlayer& operator=(FUniqueNetIdZomboyPlayer&&) = default;
	FUniqueNetIdZomboyPlayer& operator=(const FUniqueNetIdZomboyPlayer&) = default;

	virtual ~FUniqueNetIdZomboyPlayer() = default;

	/**
	 * Constructs this object with the specified net id
	 *
	 * @param InUniqueNetId the id to set ours to
	 */
	explicit FUniqueNetIdZomboyPlayer(uint64 InUniqueNetId)
		: UniqueNetId64(InUniqueNetId)
	{
	}

	/**
	 * Constructs this object with the specified net id
	 *
	 * @param String textual representation of an id
	 */
	explicit FUniqueNetIdZomboyPlayer(const FString& Str) :
		UniqueNetId64(FCString::Atoi64(*Str))
	{
	}

	/**
	 * Constructs this object with the specified net id
	 *
	 * @param InUniqueNetId the id to set ours to (assumed to be FUniqueNetIdSteam in fact)
	 */
	explicit FUniqueNetIdZomboyPlayer(const FUniqueNetId& InUniqueNetId) :
		UniqueNetId64(*(uint64*)InUniqueNetId.GetBytes())
	{
	}

	// IOnlinePlatformData

	virtual const uint8* GetBytes() const override
	{
		//return (const uint8*)UniqueNetIdStr.GetCharArray().GetData();
		return (uint8*)&UniqueNetId64;
	}

	virtual int32 GetSize() const override
	{
		return sizeof(uint64);
	}

	virtual bool IsValid() const override;


	virtual FString ToString() const override
	{
		return FString::Printf(TEXT("%llu"), UniqueNetId64);
	}

	virtual FString ToDebugString() const override
	{
		return FString::Printf(TEXT("Debug: %llu"), UniqueNetId64);
	}

	/** Needed for TMap::GetTypeHash() */
	friend uint32 GetTypeHash(const FUniqueNetIdZomboyPlayer& A)
	{
		return (uint32)(A.UniqueNetId64) + ((uint32)((A.UniqueNetId64) >> 32) * 23);
	}

};

/**
* Implementation of session information
*/
class FOnlineSessionInfoZomboy : public FOnlineSessionInfo
{
protected:

	/** Hidden on purpose */
	FOnlineSessionInfoZomboy(const FOnlineSessionInfoZomboy& Src)
	{
	}

	/** Hidden on purpose */
	FOnlineSessionInfoZomboy& operator=(const FOnlineSessionInfoZomboy& Src)
	{
		return *this;
	}

PACKAGE_SCOPE:

	/** Constructor */
	FOnlineSessionInfoZomboy();

	/**
	* Initialize a Null session info with the address of this machine
	* and an id for the session
	*/
	void Init(const class FOnlineSubsystemZomboy& Subsystem);

	/** The ip & port that the host is listening on (valid for LAN/GameServer) */
	TSharedPtr<class FInternetAddr> HostAddr;
	/** Unique Id for this session */
	FUniqueNetIdString SessionId;

	EGameLiftRegion SessionRegion;

	bool bFromMatchmaking = false;
	bool bIsHost = false;
	FString GameLiftPlayerId;
	EGameLiftRegion GameLiftPlayerRegion;
	FString GameLiftMatchmakingTicket;


public:

	virtual ~FOnlineSessionInfoZomboy() {}

	bool operator==(const FOnlineSessionInfoZomboy& Other) const
	{
		return false;
	}

	virtual const uint8* GetBytes() const override
	{
		return NULL;
	}

	virtual int32 GetSize() const override
	{
		return sizeof(uint64) + sizeof(TSharedPtr<class FInternetAddr>);
	}

	virtual bool IsValid() const override
	{
		// LAN case
		return HostAddr.IsValid() && HostAddr->IsValid();
	}

	virtual FString ToString() const override
	{
		return SessionId.ToString();
	}

	virtual FString ToDebugString() const override
	{
		return FString::Printf(TEXT("HostIP: %s SessionId: %s"),
			HostAddr.IsValid() ? *HostAddr->ToString(true) : TEXT("INVALID"),
			*SessionId.ToDebugString());
	}

	virtual const FUniqueNetId& GetSessionId() const override
	{
		return SessionId;
	}
};

UENUM(BlueprintType)
enum class EGameLiftGameSessionSettingType : uint8
{
	EGameLiftGameSessionSettingType_Dynamic		UMETA(DisplayName = "Dynamic"),
	EGameLiftGameSessionSettingType_Static		UMETA(DisplayName = "Static")
};

USTRUCT(BlueprintType)
struct FGameLiftGameSessionSetting
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
	FString Value;

	UPROPERTY(BlueprintReadWrite)
	EGameLiftGameSessionSettingType Type;

public:
	FGameLiftGameSessionSetting() :
		Value(FString()),
		Type(EGameLiftGameSessionSettingType::EGameLiftGameSessionSettingType_Dynamic)
	{

	}

	FGameLiftGameSessionSetting(const FString& InValue, EGameLiftGameSessionSettingType InType) :
		Value(InValue),
		Type(InType)
	{

	}
};


UENUM(BlueprintType)
enum class EGameLiftPlayerSessionCreationPolicy : uint8
{
	AcceptAll,
	DenyAll
};

USTRUCT(BlueprintType)
struct FGameLiftServerGameSession
{
	GENERATED_USTRUCT_BODY();
public:

	UPROPERTY(BlueprintReadOnly)
	FString SessionId;

	UPROPERTY(BlueprintReadOnly)
	EGameLiftRegion Region;

	UPROPERTY(BlueprintReadOnly)
	bool bIsMatchmakingServer;

	UPROPERTY(BlueprintReadOnly)
	TMap<FName, FString> DynamicProperties;

	UPROPERTY(BlueprintReadOnly)
	TMap<FName, FString> StaticProperties;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxPlayers;

	UPROPERTY(BlueprintReadOnly)
	EGameLiftPlayerSessionCreationPolicy PlayerSessionCreationPolicy;

	UPROPERTY()
	FString MatchmakingData;

	UPROPERTY()
	FString MatchmakingConfigurationArn;

public:

	FString GetGameSessionDynamicProperty(const FName& Key) const
	{
		const FString* Result = DynamicProperties.Find(Key);

		return Result != NULL ? *Result : FString();
	}

	FString GetGameSessionStaticProperty(const FName& Key) const
	{
		const FString* Result = StaticProperties.Find(Key);

		return Result != NULL ? *Result : FString();
	}
};

USTRUCT(BlueprintType)
struct FGameLiftSessionResult
{
	GENERATED_USTRUCT_BODY()

public:
	FOnlineSessionSearchResult SearchResult;

	UPROPERTY(BlueprintReadOnly)
	EGameLiftRegion Region;

	UPROPERTY(BlueprintReadOnly)
	int PlayerCount;

	UPROPERTY(BlueprintReadOnly)
	int PlayerCapacity;

	UPROPERTY(BlueprintReadOnly)
	TMap<FName, FString> DynamicProperties;

	UPROPERTY(BlueprintReadOnly)
	TMap<FName, FString> StaticProperties;
};

USTRUCT(BlueprintType)
struct FZomboyOnlinePlayerIconInfo
{
	GENERATED_USTRUCT_BODY()

public:

	FZomboyOnlinePlayerIconInfo()
	{

	}

	FZomboyOnlinePlayerIconInfo(int16 InWidth, int16 InHeight, const TArray<uint8>& InImageData) :
		Width(InWidth),
		Height(InHeight),
		ImageData(InImageData)
	{

	}

	UPROPERTY()
	int Width = 0;
	UPROPERTY()
	int Height = 0;

	TArray<uint8> ImageData;

	bool IsValid() const
	{
		return (Width * Height * 4 == ImageData.Num())
			&& Width != 0 && Height != 0;
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << Width;
		Ar << Height;

		if (!Ar.IsLoading())
		{
			if (Width * Height * 4 != ImageData.Num())
			{
				bOutSuccess = false;
				return true;
			}
		}
		else
		{
			ImageData.SetNum(4 * Width * Height, false);
		}

		uint8* Data = ImageData.GetData();
		Ar.ByteOrderSerialize(Data, 4 * Width * Height);

		bOutSuccess = true;
		return true;
	}

};


template<>
struct TStructOpsTypeTraits<FZomboyOnlinePlayerIconInfo> : public TStructOpsTypeTraitsBase2<FZomboyOnlinePlayerIconInfo>
{
	enum
	{
		WithNetSerializer = true
	};
};

UCLASS()
class UZomboyOSSFunctionLibrary : public UObject
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintPure)
	static bool IsValid(const FZomboyOnlinePlayerIconInfo& PlayerIconInfo);

	UFUNCTION(BlueprintPure)
	static FString GetPlayerOnlineNickName();

	UFUNCTION(BlueprintCallable)
	static UTexture2DDynamic* GetPlayerIconTexture2DDynamic(const FZomboyOnlinePlayerIconInfo& PlayerIconInfo);

	UFUNCTION(BlueprintCallable)
	static EGameLiftRegion GetGameLiftClientRegion();

	UFUNCTION(BlueprintCallable)
	static void SetGameLiftClientRegion(EGameLiftRegion Region);

};