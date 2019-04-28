// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class OnlineSubsystemZomboy : ModuleRules
{
	public OnlineSubsystemZomboy(ReadOnlyTargetRules Target) : base(Target)
    {
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDefinitions.Add("ONLINESUBSYSTEMZOMBOY_PACKAGE=1");

        PublicDependencyModuleNames.AddRange(
			new string[] {
                "OnlineSubsystemUtils"
                }
			);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
				"CoreUObject", 
				"Engine", 
				"Sockets", 
                "Voice",
				"OnlineSubsystem", 
				"Json",
                "RenderCore",
                "RHI",
                "Http"
            }
            );

        string TargetPlatform = "Steam";
       // string TargetPlatform = "Oculus";

        PublicDependencyModuleNames.AddRange(new string[] { "GameLiftClientSDK" });


        if (Target.Type == TargetRules.TargetType.Server)
        {
            PublicDefinitions.Add("WITH_GAMELIFT_SERVER=1");

            PublicDefinitions.Add("WITH_STEAM=0");
            PublicDefinitions.Add("WITH_OCULUS=0");



            PublicDependencyModuleNames.AddRange(new string[] { "GameLiftServerSDK" });
        }
        else
        {
            PublicDefinitions.Add("WITH_GAMELIFTCLIENTSDK=1");
            PublicDefinitions.Add("WITH_GAMELIFT_SERVER=0");

            //Just for editing
            //PublicDefinitions.Add("WITH_GAMELIFT_SERVER=1");
            //PublicDependencyModuleNames.AddRange(new string[] { "GameLiftServerSDK" });

            if (TargetPlatform == "Steam")
            {
                PublicDefinitions.Add("WITH_STEAM=1");
                PublicDefinitions.Add("WITH_OCULUS=0");

                string SteamVersion = "Steamv139";
                bool bSteamSDKFound = Directory.Exists(Target.UEThirdPartySourceDirectory + "Steamworks/" + SteamVersion) == true;

                PublicDefinitions.Add("STEAMSDK_FOUND=" + (bSteamSDKFound ? "1" : "0"));
                PublicDefinitions.Add("WITH_STEAMWORKS=" + (bSteamSDKFound ? "1" : "0"));

                AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");
            }
            else if (TargetPlatform == "Oculus")
            {
                PublicDefinitions.Add("WITH_STEAM=0");
                PublicDefinitions.Add("WITH_OCULUS=1");

                PublicDependencyModuleNames.AddRange(new string[] { "LibOVRPlatform" });

                if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32)
                {
                    if (Target.Platform == UnrealTargetPlatform.Win32)
                    {
                        PublicDelayLoadDLLs.Add("LibOVRPlatform32_1.dll");
                    }
                    else
                    {
                        PublicDelayLoadDLLs.Add("LibOVRPlatform64_1.dll");
                    }
                }
            }
        }
    }
}
