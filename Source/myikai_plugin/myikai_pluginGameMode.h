// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "myikai_pluginGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class Amyikai_pluginGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	/** Constructor */
	Amyikai_pluginGameMode();
};
