// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.


#include "AbilitySystem/ChronoGhostComponent.h"
#include "AbilitySystem/ChronoGhost.h"
#include "GameFramework/Character.h"

UChronoGhostComponent::UChronoGhostComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UChronoGhostComponent::EnsurePool()
{
	if (Pool.Num() > 0) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Pool.Reserve(PoolSize);
	for (int32 i = 0; i < PoolSize; i++)
	{
		AChronoGhost* Ghost = World->SpawnActor<AChronoGhost>(AChronoGhost::StaticClass(), FTransform::Identity, Params);
		if (Ghost) 
		{
			Ghost->SetGhostMaterial(GhostMaterial);
		}
		Pool.Add(Ghost);
	}
}

void UChronoGhostComponent::SpawnGhost()
{
	EnsurePool();

	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || Pool.Num() == 0) return;
	
	// Round robin
	AChronoGhost* Ghost = Pool[NextGhostIndex];
	NextGhostIndex = (NextGhostIndex + 1) % Pool.Num();

	if (Ghost)
	{
		Ghost->Show(OwnerCharacter->GetMesh(), GhostLifetime);
	}
}

void UChronoGhostComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (AChronoGhost* Ghost : Pool)
	{
		if (IsValid(Ghost))
		{
			Ghost->Destroy();
		}
	}

	Pool.Reset();

	Super::EndPlay(EndPlayReason);
}
