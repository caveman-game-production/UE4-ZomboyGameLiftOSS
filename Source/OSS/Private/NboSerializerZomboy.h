// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemZomboyTypes.h"
#include "NboSerializer.h"

/**
 * Serializes data in network byte order form into a buffer
 */
class FNboSerializeToBufferZomboy : public FNboSerializeToBuffer
{
public:
	/** Default constructor zeros num bytes*/
	FNboSerializeToBufferZomboy() :
		FNboSerializeToBuffer(512)
	{
	}

	/** Constructor specifying the size to use */
	FNboSerializeToBufferZomboy(uint32 Size) :
		FNboSerializeToBuffer(Size)
	{
	}

	/**
	 * Adds Zomboy session info to the buffer
	 */
 	friend inline FNboSerializeToBufferZomboy& operator<<(FNboSerializeToBufferZomboy& Ar, const FOnlineSessionInfoZomboy& SessionInfo)
 	{
		check(SessionInfo.HostAddr.IsValid());
		// Skip SessionType (assigned at creation)
		Ar << SessionInfo.SessionId;
		Ar << *SessionInfo.HostAddr;
		return Ar;
 	}

	/**
	 * Adds Zomboy Unique Id to the buffer
	 */
	friend inline FNboSerializeToBufferZomboy& operator<<(FNboSerializeToBufferZomboy& Ar, const FUniqueNetId& UniqueId)
	{
		Ar << UniqueId.ToString();
		return Ar;
	}
};

/**
 * Class used to write data into packets for sending via system link
 */
class FNboSerializeFromBufferZomboy : public FNboSerializeFromBuffer
{
public:
	/**
	 * Initializes the buffer, size, and zeros the read offset
	 */
	FNboSerializeFromBufferZomboy(uint8* Packet,int32 Length) :
		FNboSerializeFromBuffer(Packet,Length)
	{
	}

	/**
	 * Reads Zomboy session info from the buffer
	 */
 	friend inline FNboSerializeFromBufferZomboy& operator>>(FNboSerializeFromBufferZomboy& Ar, FOnlineSessionInfoZomboy& SessionInfo)
 	{
		check(SessionInfo.HostAddr.IsValid());
		// Skip SessionType (assigned at creation)
		Ar >> SessionInfo.SessionId; 
		Ar >> *SessionInfo.HostAddr;
		return Ar;
 	}

	/**
	 * Reads Zomboy Unique Id from the buffer
	 */
	friend inline FNboSerializeFromBufferZomboy& operator>>(FNboSerializeFromBufferZomboy& Ar, FUniqueNetId& UniqueId)
	{
		FString UniqueIdString = UniqueId.ToString();
		Ar >> UniqueIdString;
		return Ar;
	}
};
