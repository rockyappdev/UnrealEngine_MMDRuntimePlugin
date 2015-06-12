// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MMDRuntimePrivatePCH.h"


class FMMDRuntime : public IMMDRuntime
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FMMDRuntime, MMDRuntime )



void FMMDRuntime::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
}


void FMMDRuntime::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}



