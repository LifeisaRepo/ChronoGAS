// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Looping.h"
#include "ChronoRewindCue.generated.h"

class UChronoGhostComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USkeletalMeshComponent;

/**
 * Handles rewind cosmetics.
 * Expects the hold duration in Parameters.RawMagnitude
 * and rewing duration in Parameters.NormalizedMagnitude.
 */
UCLASS()
class CHRONOGAS_API AChronoRewindCue : public AGameplayCueNotify_Looping
{
	GENERATED_BODY()

public:

	virtual void Tick(float DeltaTime) override;

protected:

	virtual bool WhileActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;

private:

	UPROPERTY(EditDefaultsOnly, Category = "Ghosts", meta = (ClampMin = "0.01"))
	float GhostInterval = 0.08f;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UMaterialInterface> OverlayMaterial;

	UPROPERTY()
	TObjectPtr<UChronoGhostComponent> Ghosts;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> GlowMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseOverlayMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> OverlayMID;

	// Real-time, unaffected by time dilation.
	double NextGhostRealtime = 0.8;
	double GlowFadeStartRealtime = 0.0;
	double GlowFadeEndRealtime = 0.0;
	
};
