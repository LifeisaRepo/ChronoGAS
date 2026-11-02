// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Burst.h"
#include "ChronoBlastCue.generated.h"

/**
 * One-shot blast visual.
 * Expects blast point in Parameters.Location, radius in Parameters.RawMagnitude,
 * and power (0-1) in Parameters.NormalizedMagnitude.
 */
UCLASS()
class CHRONOGAS_API UChronoBlastCue : public UGameplayCueNotify_Burst
{
	GENERATED_BODY()

protected:

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	
};
