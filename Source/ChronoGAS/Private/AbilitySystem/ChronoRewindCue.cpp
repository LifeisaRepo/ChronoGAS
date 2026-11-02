// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.


#include "AbilitySystem/ChronoRewindCue.h"
#include "AbilitySystem/ChronoGhostComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"

static const FName OverlayOpacityParameterName(TEXT("Opacity"));

bool AChronoRewindCue::WhileActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	const bool bResult = Super::WhileActive_Implementation(Target, Parameters);

	const double Now = GetWorld()->GetRealTimeSeconds();
	const float HoldDuration = Parameters.RawMagnitude;
	const float RewindDuration = Parameters.NormalizedMagnitude;

	Ghosts = Target ? Target->FindComponentByClass<UChronoGhostComponent>() : nullptr;
	if (Ghosts)
	{
		// Create the ghost pool during the blast.
		Ghosts->EnsurePool();

		// No ghosts during the initial hold duration (Parameters.RawMagnitude).
		NextGhostRealtime = Now + HoldDuration + GhostInterval;
	}

	const ACharacter* Character = Cast<ACharacter>(Target);
	GlowMesh = Character ? Character->GetMesh() : nullptr;

	if (GlowMesh && OverlayMaterial)
	{
		BaseOverlayMaterial = GlowMesh->GetOverlayMaterial();
		OverlayMID = UMaterialInstanceDynamic::Create(OverlayMaterial, this);
		OverlayMID->SetScalarParameterValue(OverlayOpacityParameterName, 1.f);
		GlowMesh->SetOverlayMaterial(OverlayMID);

		// Fade out over last 30% of the rewind
		GlowFadeStartRealtime = Now + HoldDuration + 0.7 * RewindDuration;
		GlowFadeEndRealtime = Now + HoldDuration + RewindDuration;
	}

	SetActorTickEnabled(true);

	return bResult;
}

bool AChronoRewindCue::OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	// Target can be null here, and this actor gets recycled, so leave nothing running
	SetActorTickEnabled(false);
	Ghosts = nullptr;

	if (OverlayMID && IsValid(GlowMesh))
	{
		GlowMesh->SetOverlayMaterial(BaseOverlayMaterial);
	}
	GlowMesh = nullptr;
	BaseOverlayMaterial = nullptr;
	OverlayMID = nullptr;

	return Super::OnRemove_Implementation(Target, Parameters);
}

void AChronoRewindCue::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const double Now = GetWorld()->GetRealTimeSeconds();
	if (Ghosts && Now >= NextGhostRealtime)
	{
		NextGhostRealtime = Now + GhostInterval;
		Ghosts->SpawnGhost();
	}

	if (OverlayMID)
	{
		const double FadeAlpha = FMath::SmoothStep(GlowFadeStartRealtime, GlowFadeEndRealtime, Now);
		OverlayMID->SetScalarParameterValue(OverlayOpacityParameterName, static_cast<float>(1.0 - FadeAlpha));
	}
}
