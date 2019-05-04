// Created by YetiTech Studios.

#pragma once

#if WITH_GAMELIFTCLIENTSDK
#include "aws/gamelift/GameLiftClient.h"
#endif

#include "GameLiftClientTypes.generated.h"

UENUM(BlueprintType)
enum class EGameLiftGameSessionStatus : uint8
{
	STATUS_Activating		UMETA(DisplayName = "Activating"),
	STATUS_Active			UMETA(DisplayName = "Active"),
	STATUS_Error			UMETA(DisplayName = "Error"),
	STATUS_NotSet			UMETA(DisplayName = "Not Set"),
	STATUS_Terminating		UMETA(DisplayName = "Terminating"),
	STATUS_Terminated		UMETA(DisplayName = "Terminated"),
	STATUS_NoStatus			UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EGameLiftRegion : uint8
{
	EGameLiftRegion_None	UMETA(DisplayName = "None"),
	EGameLiftRegion_NA		UMETA(DisplayName = "NA"),
	EGameLiftRegion_EU		UMETA(DisplayName = "EU"),
	EGameLiftRegion_APJ		UMETA(DisplayName = "APJ"),
};

constexpr TCHAR* GetRegionString(EGameLiftRegion Region)
{
	switch (Region)
	{
		case EGameLiftRegion::EGameLiftRegion_NA:
		{
			return TEXT("us-east-1");
		}
		break;

		case EGameLiftRegion::EGameLiftRegion_EU:
		{
			return TEXT("eu-central-1");
		}
		break;

		case EGameLiftRegion::EGameLiftRegion_APJ:
		{
			return TEXT("ap-northeast-1");
		}
		break;
	}

	return TEXT("us-east-1");
}

inline EGameLiftRegion GetRegion(const TCHAR* RegionString)
{
	if (FCString::Strcmp(RegionString, TEXT("us-east-1")) == 0)
	{
		return EGameLiftRegion::EGameLiftRegion_NA;
	}
	else if (FCString::Strcmp(RegionString, TEXT("eu-central-1")) == 0)
	{
		return EGameLiftRegion::EGameLiftRegion_EU;
	}
	else if (FCString::Strcmp(RegionString, TEXT("ap-northeast-1")) == 0)
	{
		return EGameLiftRegion::EGameLiftRegion_APJ;
	}

	return EGameLiftRegion::EGameLiftRegion_None;
}

static Aws::Map<Aws::String, int> GetRegionLatencyAWS(EGameLiftRegion Region)
{
	Aws::Map<Aws::String, int> LatencyMap;
	switch (Region)
	{
	case EGameLiftRegion::EGameLiftRegion_NA:
	{
		LatencyMap.emplace(TCHAR_TO_UTF8(GetRegionString(EGameLiftRegion::EGameLiftRegion_NA)), 30);
		LatencyMap.emplace(TCHAR_TO_UTF8(GetRegionString(EGameLiftRegion::EGameLiftRegion_EU)), 100);
		LatencyMap.emplace(TCHAR_TO_UTF8(GetRegionString(EGameLiftRegion::EGameLiftRegion_APJ)), 120);
	}
	break;

	case EGameLiftRegion::EGameLiftRegion_EU:
	{
		LatencyMap.emplace(TCHAR_TO_UTF8(GetRegionString(EGameLiftRegion::EGameLiftRegion_EU)), 30);
		LatencyMap.emplace(TCHAR_TO_UTF8(GetRegionString(EGameLiftRegion::EGameLiftRegion_NA)), 100);
		LatencyMap.emplace(TCHAR_TO_UTF8(GetRegionString(EGameLiftRegion::EGameLiftRegion_APJ)), 120);
	}
	break;

	case EGameLiftRegion::EGameLiftRegion_APJ:
	{
		LatencyMap.emplace(TCHAR_TO_UTF8(GetRegionString(EGameLiftRegion::EGameLiftRegion_APJ)), 30);
		LatencyMap.emplace(TCHAR_TO_UTF8(GetRegionString(EGameLiftRegion::EGameLiftRegion_NA)), 100);
		LatencyMap.emplace(TCHAR_TO_UTF8(GetRegionString(EGameLiftRegion::EGameLiftRegion_EU)), 120);
	}
	break;

	}

	return LatencyMap;
}

static TMap<FString, int32> GetRegionLatency(EGameLiftRegion Region)
{
	TMap<FString, int32> LatencyMap;
	switch (Region)
	{
	case EGameLiftRegion::EGameLiftRegion_NA:
	{
		LatencyMap.Emplace(GetRegionString(EGameLiftRegion::EGameLiftRegion_NA), 30);
		LatencyMap.Emplace(GetRegionString(EGameLiftRegion::EGameLiftRegion_EU), 100);
		LatencyMap.Emplace(GetRegionString(EGameLiftRegion::EGameLiftRegion_APJ), 120);
	}
	break;

	case EGameLiftRegion::EGameLiftRegion_EU:
	{
		LatencyMap.Emplace(GetRegionString(EGameLiftRegion::EGameLiftRegion_EU), 30);
		LatencyMap.Emplace(GetRegionString(EGameLiftRegion::EGameLiftRegion_NA), 100);
		LatencyMap.Emplace(GetRegionString(EGameLiftRegion::EGameLiftRegion_APJ), 120);
	}
	break;

	case EGameLiftRegion::EGameLiftRegion_APJ:
	{
		LatencyMap.Emplace(GetRegionString(EGameLiftRegion::EGameLiftRegion_APJ), 30);
		LatencyMap.Emplace(GetRegionString(EGameLiftRegion::EGameLiftRegion_NA), 100);
		LatencyMap.Emplace(GetRegionString(EGameLiftRegion::EGameLiftRegion_EU), 120);
	}
	break;
	}

	return LatencyMap;
}

USTRUCT(Blueprintable, BlueprintType)
struct FGameLiftGameSessionServerProperties
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString Key;

	UPROPERTY(BlueprintReadWrite)
	FString Value;
};


USTRUCT(BlueprintType)
struct FGameLiftGameSessionConfig
{
	GENERATED_USTRUCT_BODY()

private:

	/* Maximum number of players that can be connected simultaneously to the game session. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int32 MaxPlayers;

	/* Set of developer-defined properties for a game session, formatted as a set of type:value pairs. 
	 * These properties are included in the GameSession object, which is passed to the game server with a request to start a new game session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TArray<FGameLiftGameSessionServerProperties> GameSessionDynamicProperties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TArray<FGameLiftGameSessionServerProperties> GameSessionStaticProperties;

public:

	void SetMaxPlayers(int32 NewMaxPlayers)
	{
		MaxPlayers = NewMaxPlayers;
	}

	void SetGameSessionDynamicProperties(const TArray<FGameLiftGameSessionServerProperties>& NewProperties)
	{
		GameSessionDynamicProperties = NewProperties;
	}

	void SetGameSessionStaticProperties(const TArray<FGameLiftGameSessionServerProperties>& NewProperties)
	{
		GameSessionStaticProperties = NewProperties;
	}

	FORCEINLINE int32 GetMaxPlayers() const { return MaxPlayers; }
	FORCEINLINE TArray<FGameLiftGameSessionServerProperties> GetGameSessionDynamicProperties() const { return GameSessionDynamicProperties; }
	FORCEINLINE TArray<FGameLiftGameSessionServerProperties> GetGameSessionStaticProperties() const { return GameSessionStaticProperties; }


	FGameLiftGameSessionConfig()
	{
		MaxPlayers = 0;
	}	
};



USTRUCT(BlueprintType)
struct FGameLiftGameSessionSearchSettings
{
	GENERATED_USTRUCT_BODY()

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FString SessionQueueName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FString SearchFilterString;

public:

	void SetSessionQueueName(FString InSessionQueueName) { SessionQueueName = InSessionQueueName; }
	void AddSearchFilter(const FString& SearchFilter) { SearchFilterString.Append(SearchFilter); }

	FORCEINLINE FString GetSessionQueueName() const { return SessionQueueName; }
	FORCEINLINE char* GetFilterString() const { return TCHAR_TO_UTF8(*SearchFilterString); }
	
};

USTRUCT(BlueprintType)
struct FGameLiftGameSessionSearchResult
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(BlueprintReadOnly)
	FString SessionId;

	UPROPERTY(BlueprintReadOnly)
	EGameLiftRegion SessionRegion;

	UPROPERTY(BlueprintReadOnly)
	FString MatchmakerData;

	UPROPERTY(BlueprintReadOnly)
	TMap<FString, FString> DynamicProperties;

	UPROPERTY(BlueprintReadOnly)
	TMap<FString, FString> StaticProperties;

	UPROPERTY(BlueprintReadOnly)
	int CurrentPlayerCount;

	UPROPERTY(BlueprintReadOnly)
	int MaxPlayerCount;
};

enum class FClientAttributeType : uint8
{
	NONE = 0,
	STRING = 1,
	DOUBLE = 2,
	STRING_LIST = 3,
	STRING_DOUBLE_MAP = 4
};

struct FClientAttributeValue {
	FString m_S;
	double m_N;
	TArray<FString> m_SL;
	TMap<FString, double> m_SDM;
	FClientAttributeType m_type;
};