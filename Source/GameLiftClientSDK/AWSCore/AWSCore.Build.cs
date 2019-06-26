using UnrealBuildTool;
using System.IO;

public class AWSCore : ModuleRules
{
	public AWSCore(ReadOnlyTargetRules Target ): base(Target)
	{
		PrivateIncludePaths.AddRange(new string[] { "GameLiftClientSDK/AWSCore/Private" });
		PublicIncludePaths.AddRange(new string[] { "GameLiftClientSDK/AWSCore/Public" });

		PublicDependencyModuleNames.AddRange(new string[] { "Engine", "Core", "CoreUObject", "InputCore", "Projects"});
		PrivateDependencyModuleNames.AddRange(new string[] { });

		string BaseDirectory = System.IO.Path.GetFullPath(System.IO.Path.Combine(ModuleDirectory, "..", "..", ".."));
        string ThirdPartyPath = System.IO.Path.Combine(BaseDirectory, "ThirdParty", "GameLiftClientSDK", Target.Platform.ToString());
        bool bIsThirdPartyPathValid = System.IO.Directory.Exists(ThirdPartyPath);

		if (bIsThirdPartyPathValid)
		{
			PublicLibraryPaths.Add(ThirdPartyPath);

            //string AWSCCommonLibFile = System.IO.Path.Combine(ThirdPartyPath, "aws-c-common.lib");
            //if (File.Exists(AWSCCommonLibFile))
            //{
            //    PublicAdditionalLibraries.Add(AWSCCommonLibFile);
            //}
            //else
            //{
            //    throw new BuildException("aws-c-common.lib not found. Expected in this location: " + AWSCCommonLibFile);
            //}

            //string AWSCCommonDLLFile = System.IO.Path.Combine(ThirdPartyPath, "aws-c-common.dll");
            //if (File.Exists(AWSCCommonDLLFile))
            //{
            //    PublicAdditionalLibraries.Add(AWSCCommonDLLFile);
            //}
            //else
            //{
            //    throw new BuildException("aws-c-common.dll not found. Expected in this location: " + AWSCCommonDLLFile);
            //}

            //string AWSCEventStreamLibFile = System.IO.Path.Combine(ThirdPartyPath, "aws-c-event-stream.lib");
            //if (File.Exists(AWSCEventStreamLibFile))
            //{
            //    PublicAdditionalLibraries.Add(AWSCEventStreamLibFile);
            //}
            //else
            //{
            //    throw new BuildException("aws-c-event-stream.lib not found. Expected in this location: " + AWSCEventStreamLibFile);
            //}

            //string AWSCEventStreamDLLFile = System.IO.Path.Combine(ThirdPartyPath, "aws-c-event-stream.dll");
            //if (File.Exists(AWSCEventStreamDLLFile))
            //{
            //    PublicAdditionalLibraries.Add(AWSCEventStreamDLLFile);
            //}
            //else
            //{
            //    throw new BuildException("aws-c-event-stream.dll not found. Expected in this location: " + AWSCEventStreamDLLFile);
            //}

            string AWSCoreLibFile = System.IO.Path.Combine(ThirdPartyPath, "aws-cpp-sdk-core.lib");
			if (File.Exists(AWSCoreLibFile))
			{
				PublicAdditionalLibraries.Add(AWSCoreLibFile);
			}
			else
			{
				throw new BuildException("aws-cpp-sdk-core.lib not found. Expected in this location: " + AWSCoreLibFile);
			}

			string AWSCoreDLLFile = System.IO.Path.Combine(ThirdPartyPath, "aws-cpp-sdk-core.dll");
            string AWSCommonDLLFile = System.IO.Path.Combine(ThirdPartyPath, "aws-c-common.dll");
            string AWSStreamDLLFile = System.IO.Path.Combine(ThirdPartyPath, "aws-c-event-stream.dll");
            string AWSChecksumDLLFile = System.IO.Path.Combine(ThirdPartyPath, "aws-checksums.dll");

            if (File.Exists(AWSCoreDLLFile))
			{
                RuntimeDependencies.Add(AWSCommonDLLFile);
                RuntimeDependencies.Add(AWSStreamDLLFile);
                RuntimeDependencies.Add(AWSChecksumDLLFile);

                PublicDelayLoadDLLs.Add("aws-cpp-sdk-core.dll");
                RuntimeDependencies.Add(AWSCoreDLLFile);
            }
            else
			{
				throw new BuildException("aws-cpp-sdk-core.dll not found. Expected in this location: " + AWSCoreDLLFile);
			}

			string BinariesDirectory = System.IO.Path.Combine(BaseDirectory, "Binaries", Target.Platform.ToString());
			if (!Directory.Exists(BinariesDirectory))
			{
				Directory.CreateDirectory(BinariesDirectory);
			}

			if (File.Exists(System.IO.Path.Combine(BinariesDirectory, "aws-cpp-sdk-core.dll")) == false)
			{
				File.Copy(System.IO.Path.Combine(ThirdPartyPath, "aws-cpp-sdk-core.dll"), System.IO.Path.Combine(BinariesDirectory, "aws-cpp-sdk-core.dll"));
			}

            if (File.Exists(System.IO.Path.Combine(BinariesDirectory, "aws-c-common.dll")) == false)
            {
                File.Copy(System.IO.Path.Combine(ThirdPartyPath, "aws-c-common.dll"), System.IO.Path.Combine(BinariesDirectory, "aws-c-common.dll"));
            }

            if (File.Exists(System.IO.Path.Combine(BinariesDirectory, "aws-c-event-stream.dll")) == false)
            {
                File.Copy(System.IO.Path.Combine(ThirdPartyPath, "aws-c-event-stream.dll"), System.IO.Path.Combine(BinariesDirectory, "aws-c-event-stream.dll"));
            }

            if (File.Exists(System.IO.Path.Combine(BinariesDirectory, "aws-checksums.dll")) == false)
            {
                File.Copy(System.IO.Path.Combine(ThirdPartyPath, "aws-checksums.dll"), System.IO.Path.Combine(BinariesDirectory, "aws-checksums.dll"));
            }
        }
    }
}
