// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.


#include "AbilitySystem/ChronoBlastCue.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

bool UChronoBlastCue::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	const bool bResult = Super::OnExecute_Implementation(MyTarget, Parameters);

	const UWorld* World = MyTarget ? MyTarget->GetWorld() : GetWorld();
	if (World)
	{
		const FColor Color = FLinearColor::LerpUsingHSV(FLinearColor::Green, FLinearColor::Red, Parameters.NormalizedMagnitude).ToFColor(true);
		DrawDebugSphere(World, Parameters.Location, Parameters.RawMagnitude, 24, Color, false, 5.f, 0, 2.f);
	}

	return bResult;
}
