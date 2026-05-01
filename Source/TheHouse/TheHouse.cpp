// Copyright Epic Games, Inc. All Rights Reserved.

#include "TheHouse.h"

#include "Modules/ModuleManager.h"
#include "Misc/CoreDelegates.h"

#include "TheHouseGraphicsProfile.h"

namespace
{

FDelegateHandle GTheHousePostEngineInitHandle;

} // namespace

class FTheHouseModule final : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();

		GTheHousePostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddLambda([]()
		{
			const ETheHouseGraphicsProfile P = TheHouseGraphicsProfile::ResolveStartupProfile();
			TheHouseGraphicsProfile::ApplyIniProfile(P);
		});
	}

	virtual void ShutdownModule() override
	{
		if (GTheHousePostEngineInitHandle.IsValid())
		{
			FCoreDelegates::OnPostEngineInit.Remove(GTheHousePostEngineInitHandle);
			GTheHousePostEngineInitHandle.Reset();
		}
		FDefaultGameModuleImpl::ShutdownModule();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FTheHouseModule, TheHouse, "TheHouse");
