// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.


#include "AbilitySystem/ChronoGhost.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

// Because creating a new FName in Tick() is expensive
static const FName OpacityParameterName(TEXT("Opacity"));

// Sets default values
AChronoGhost::AChronoGhost()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	PoseableMesh = CreateDefaultSubobject<UPoseableMeshComponent>("PoseableMesh");
	SetRootComponent(PoseableMesh);
	PoseableMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PoseableMesh->SetCastShadow(false);

	SetActorHiddenInGame(true);

}

void AChronoGhost::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const double Age = GetWorld()->GetRealTimeSeconds() - ShowRealTime;
	if (Age >= LifetimeSeconds)
	{
		Hide();
		return;
	}

	if (GhostMID)
	{
		// Squared fade
		const float Remaining = 1.f - static_cast<float>(Age / LifetimeSeconds);
		GhostMID->SetScalarParameterValue(OpacityParameterName, Remaining * Remaining);
	}
}

void AChronoGhost::SetGhostMaterial(UMaterialInterface* InMaterial)
{
	GhostMaterial = InMaterial;
}

void AChronoGhost::Show(USkeletalMeshComponent* Source, float Lifetime)
{
	if (!Source) return;

	if (PoseableMesh->GetSkinnedAsset() != Source->GetSkinnedAsset())
	{
		PoseableMesh->SetSkinnedAssetAndUpdate(Source->GetSkinnedAsset());
	}

	if (GhostMaterial && !GhostMID)
	{
		GhostMID = UMaterialInstanceDynamic::Create(GhostMaterial, this);
		for (int32 SlotIndex = 0; SlotIndex < PoseableMesh->GetNumMaterials(); SlotIndex++)
		{
			PoseableMesh->SetMaterial(SlotIndex, GhostMID);
		}
	}

	if (GhostMID)
	{
		GhostMID->SetScalarParameterValue(OpacityParameterName, 1.f);
	}

	SetActorTransform(Source->GetComponentTransform());
	PoseableMesh->CopyPoseFromSkeletalComponent(Source);

	ShowRealTime = GetWorld()->GetRealTimeSeconds();
	LifetimeSeconds = FMath::Max(0.01f, Lifetime);

	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
}

void AChronoGhost::Hide()
{
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
}

