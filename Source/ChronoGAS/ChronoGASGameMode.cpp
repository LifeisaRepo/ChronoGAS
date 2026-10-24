// Copyright (c) 2026 Sanjyot Dahale. All rights reserved.

#include "ChronoGASGameMode.h"
#include "ChronoGASCharacter.h"
#include "UObject/ConstructorHelpers.h"

AChronoGASGameMode::AChronoGASGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
