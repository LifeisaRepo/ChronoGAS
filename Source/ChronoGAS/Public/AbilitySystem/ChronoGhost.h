// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChronoGhost.generated.h"

class UPoseableMeshComponent;
class USkeletalMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;


/**
 * Frozen copy of a character's pose for ChronoRecall ability.
 * Pooled by UChronoGhostComponent.
 */
UCLASS()
class CHRONOGAS_API AChronoGhost : public AActor
{
	GENERATED_BODY()
	
public:	

	AChronoGhost();

	void Show(USkeletalMeshComponent* Source, float Lifetime);
	
	virtual void Tick(float DeltaTime) override;

	void SetGhostMaterial(UMaterialInterface* InMaterial);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float LifetimeSeconds = 0.4f;

private:

	void Hide();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPoseableMeshComponent> PoseableMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> GhostMaterial;

	// Ghost's own instance
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> GhostMID;

	// Real time (unaffected by time dilation) when the ghost was shown
	double ShowRealTime = 0.f;

};
