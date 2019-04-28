// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystem.h"
#include "Modules/ModuleManager.h"

#define INVALID_INDEX -1

/** URL Prefix when using Null socket connection */
#define NULL_URL_PREFIX TEXT("Zomboy.")

/** pre-pended to all NULL logging */
#undef ONLINE_LOG_PREFIX
#define ONLINE_LOG_PREFIX TEXT("Zomboy: ")

#if WITH_GAMELIFTCLIENTSDK	

#include "GameLiftClientObject.h"
#include "GameLiftClientApi.h"

#define GAMELIFT_LOCAL false

#endif

//#ifndef WITH_STEAM
//#define WITH_STEAM=1
//#endif

#if WITH_STEAM == 1

/** Root location of Steam SDK */
#define STEAM_SDK_ROOT_PATH TEXT("Binaries/ThirdParty/Steamworks")
#define STEAMAPPIDFILENAME TEXT("steam_appid.txt")

#pragma push_macro("ARRAY_COUNT")
#undef ARRAY_COUNT

// Steamworks SDK headers
#if STEAMSDK_FOUND == 0
#error Steam SDK not located.  Expected to be found in Engine/Source/ThirdParty/Steamworks/{SteamVersion}
#endif

THIRD_PARTY_INCLUDES_START

#include "steam/steam_api.h"
#include "steam/steam_gameserver.h"

THIRD_PARTY_INCLUDES_END

#pragma pop_macro("ARRAY_COUNT")

#endif

//#ifndef WITH_OCULUS
//#define WITH_OCULUS=1
//#endif

#if WITH_OCULUS

#include "OVR_Platform.h"

#endif
