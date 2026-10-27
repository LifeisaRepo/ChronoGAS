// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ChronoGhostComponent.generated.h"

class AChronoGhost;
class UMaterialInterface;

/**
 * Pool of ghost afterimages for the owning character.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHRONOGAS_API UChronoGhostComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	UChronoGhostComponent();
	
	// Spawns the pool the first time it's called.
	void EnsurePool();

	// Shows the next ghost in the pool at the owner's current pose.
	void SpawnGhost();

protected:

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	UPROPERTY(EditDefaultsOnly, Category = "Ghosts", meta = (ClampMin = "1"))
	int32 PoolSize = 8;

	// How long each ghost stays visible
	UPROPERTY(EditDefaultsOnly, Category = "Ghosts", meta = (ClampMin = "0.01"))
	float GhostLifetime = 0.4f;

	// Glowing ghost material
	// **NOTE TO SELF** = Scalar parameter for Opacity is hardcoded with "Opacity" FName
	UPROPERTY(EditDefaultsOnly, Category="Ghosts")
	TObjectPtr<UMaterialInterface> GhostMaterial;

	UPROPERTY()
	TArray<TObjectPtr<AChronoGhost>> Pool;

	int32 NextGhostIndex = 0;
		
};
