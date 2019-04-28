// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

// Can't be #pragma once because other modules may define PACKAGE_SCOPE

// Intended to be the last include in an exported class definition
// Properly defines some members as "public to the module" vs "private to the consumer/user"

#undef PACKAGE_SCOPE
#ifdef ONLINESUBSYSTEMZOMBOY_PACKAGE
#define PACKAGE_SCOPE public
#else
#define PACKAGE_SCOPE protected
#endif

#ifndef ZOMBOY_SUBSYSTEM
#define ZOMBOY_SUBSYSTEM FName(TEXT("Zomboy"))
#endif