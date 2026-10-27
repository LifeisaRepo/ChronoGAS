// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.


#include "AbilitySystem/AbilityTasks/ChronoRewind.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "TemporalBufferComponent.h"
#include "AbilitySystem/ChronoGhostComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"

static const FName OverlayOpacityParameterName(TEXT("Opacity"));

UChronoRewind::UChronoRewind()
{
	bTickingTask = true;
}

UChronoRewind* UChronoRewind::CreateChronoRewindTask(UGameplayAbility* OwningAbility, UTemporalBufferComponent* InBuffer, float InDuration, const FChronoRewindPunch& InPunch)
{
	UChronoRewind* Task = NewAbilityTask<UChronoRewind>(OwningAbility);
	Task->Buffer = InBuffer;
	Task->Duration = FMath::Max(InDuration, 0.01f);
	Task->Punch = InPunch;
	Task->Punch.HoldDuration = FMath::Max(InPunch.HoldDuration, 0.f);
	Task->Punch.GhostInterval = FMath::Max(InPunch.GhostInterval, 0.01f);
	return Task;
}

void UChronoRewind::Activate()
{
	Super::Activate();

	Character = Cast<ACharacter>(GetAvatarActor());
	if (!Character || !Buffer)
	{
		Finish();
		return;
	}
	
	Buffer->SetRecordingPaused(true);

	// No recorded path to rewind along yet.
	if (Buffer->GetNumSnapshots() < 2)
	{
		Finish();
		return;
	}

	TakeOverMovement();
	BeginPunch();

}

void UChronoRewind::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	ElapsedTime += DeltaTime;

	// Hold on the blast point for HoldDuration, then rewind
	const float RewindAlpha = FMath::Clamp((ElapsedTime - Punch.HoldDuration) / Duration, 0.f, 1.0f);
	ApplyRewindPose(RewindAlpha);
	ApplyPunch(RewindAlpha);
	UpdateCameraFocus(DeltaTime);
	UpdateGhosts(DeltaTime, RewindAlpha);

	if (ElapsedTime < Punch.HoldDuration + Duration) return;

	// Reached the oldest snapshot
	const FVector ExitVelocity = Buffer->GetSnapshotByAge(Buffer->GetNumSnapshots() - 1).Velocity;
	ReleaseMovement();

	UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
	if (!FMath::IsNearlyZero(ExitVelocity.Z, 1.f))
	{
		// ExitVelocity.Z > 0 is only possible if character was in the air during that snapshot.
		CMC->SetMovementMode(MOVE_Falling);
	}

	CMC->Velocity = ExitVelocity;

	Finish();

}

void UChronoRewind::OnDestroy(bool bInOwnerFinished)
{
	// Always runs when the task is destroyed to ensure that the character is handed back to the CMC safely.
	ReleaseMovement();
	EndPunch();

	if (Buffer)
	{
		Buffer->SetRecordingPaused(false);
	}
	
	Super::OnDestroy(bInOwnerFinished);
}

void UChronoRewind::TakeOverMovement()
{
	UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
	// 1. Stop movement
	CMC->StopMovementImmediately();	
	// 2. Set mode to flying
	CMC->SetMovementMode(MOVE_Flying);

	// During local prediction, client is always going to be ahead
	// By default, the server will keep correcting the client in-case their
	// distance differs by > 1.7cm which is much less than our high-speed rewind
	// will do per frame. Setting this flag will ensure that the server stops the corrections
	// for the time being.
	CMC->bIgnoreClientMovementErrorChecksAndCorrection = true;

	// 3. disable collisions
	Character->SetActorEnableCollision(false);

	// 4. ignore inputs from controller
	if (AController* Controller = Character->GetController())
	{
		Controller->SetIgnoreMoveInput(true);
	}

	bMovementTakenOver = true;
	// UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.1f);
}

void UChronoRewind::ReleaseMovement()
{
	// Roll-back everything we did in TakeOverMovement()
	// UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
	if (!bMovementTakenOver || !Character)
	{
		return;
	}
	bMovementTakenOver = false;

	Character->SetActorEnableCollision(true);

	UCharacterMovementComponent* CMC = Character->GetCharacterMovement();

	// Very important to disable this flag once we are done.
	CMC->bIgnoreClientMovementErrorChecksAndCorrection = false;

	CMC->SetMovementMode(MOVE_Walking);

	if (AController* Controller = Character->GetController())
	{
		Controller->SetIgnoreMoveInput(false);
	}
}

void UChronoRewind::ApplyRewindPose(float Alpha)
{
	const int32 NumSnapshots = Buffer->GetNumSnapshots();

	// 0-1 Ease in-out for the rewind
	const float EasedAlpha = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);

	// Get the eligible snapshot index as per the EasedAlpha
	const float PathPosition = EasedAlpha * (NumSnapshots - 1);
	// Floor the float value to get Snapshot A
	const int32 AgeA = FMath::Clamp(FMath::FloorToInt(PathPosition), 0, NumSnapshots - 1);
	// Get the snapshot A + 1 as well
	const int32 AgeB = FMath::Min(AgeA + 1, NumSnapshots - 1);
	// As per the original floating value, how much to A and B need to blend ?
	const float Blend = PathPosition - AgeA;
	
	// Get transforms for both A and B
	const FTransform& A = Buffer->GetSnapshotByAge(AgeA).Transform;
	const FTransform& B = Buffer->GetSnapshotByAge(AgeB).Transform;

	// Get the blended location and rotation between A and B
	const FVector Location = FMath::Lerp(A.GetLocation(), B.GetLocation(), Blend);
	const FQuat Rotation = FQuat::Slerp(A.GetRotation(), B.GetRotation(), Blend);

	// Set it
	Character->SetActorLocationAndRotation(Location, Rotation);

}

void UChronoRewind::BeginPunch()
{
	// Camera and time effects are only for owning client
	if (!Character->IsLocallyControlled()) return;
	
	Camera = Character->FindComponentByClass<UCameraComponent>();
	if (Camera)
	{
		BaseFOV = Camera->FieldOfView;
		BaseFringeIntensity = Camera->PostProcessSettings.SceneFringeIntensity;		// Chromatic aberration -_-
		bBaseFringeOverride = Camera->PostProcessSettings.bOverride_SceneFringeIntensity;
		BaseMotionBlurIntensity = Camera->PostProcessSettings.MotionBlurAmount;
		bBaseMotionBlurOverride = Camera->PostProcessSettings.bOverride_MotionBlurAmount;

		Camera->PostProcessSettings.bOverride_SceneFringeIntensity = true;
		Camera->PostProcessSettings.bOverride_MotionBlurAmount = true;
	}

	// Global time dilation cannot be used in multiplayer because it's replicated from the server,
	// so it would slow down every player... obviously! -_- . So we'll only allow it in Standalone
	bUseTimeDilation = GetWorld()->GetNetMode() == NM_Standalone;

	// CameraFocus - Keep the camera away from the mouse and keep it pointed at the blast
	if (Punch.bFocusCameraOnBlast)
	{
		LocalController = Character->GetController();
		if (LocalController)
		{
			LocalController->SetIgnoreLookInput(true);
			FocusPoint = Buffer->GetSnapshotByAge(0).Transform.GetLocation();
			FocusYaw = LocalController->GetControlRotation().Yaw;
			bCameraFocusActive = true;
		}

	}

	// Create the ghost pool during the blast - to mitigate any possible hitch
	if (Punch.bSpawnGhosts)
	{
		Ghosts = Character->FindComponentByClass<UChronoGhostComponent>();
		if (Ghosts)
		{
			Ghosts->EnsurePool();
		}
	}

	if (Punch.RewindOverlayMaterial)
	{
		USkeletalMeshComponent* Mesh = Character->GetMesh();
		BaseOverlayMaterial = Mesh->GetOverlayMaterial();
		OverlayMID = UMaterialInstanceDynamic::Create(Punch.RewindOverlayMaterial, Character);
		Mesh->SetOverlayMaterial(OverlayMID);
	}

	bPunchActive = true;

	ApplyPunch(0.f);

}

void UChronoRewind::ApplyPunch(float RewindAlpha)
{
	if (!bPunchActive) return;

	// Camera effects: 0 -> 1 -> 0 over the scrub
	const float Pulse = FMath::Max(0.f, FMath::Sin(UE_PI * RewindAlpha));

	if (OverlayMID)
	{
		const float Glow = 1.f - FMath::SmoothStep(0.7f, 1.f, RewindAlpha);
		OverlayMID->SetScalarParameterValue(OverlayOpacityParameterName, Glow);
	}

	if (Camera)
	{
		Camera->SetFieldOfView(BaseFOV + Punch.ExtraFOV * Pulse);
		Camera->PostProcessSettings.MotionBlurAmount = BaseMotionBlurIntensity + Punch.PeakMotionBlurAmount * Pulse;

		// Assuming that we are not using the camera's chromatic aberration by default.
		// Otherwise, the calculation would be dependent on the bOverride_SceneFringeIntensity flag
		Camera->PostProcessSettings.SceneFringeIntensity = Punch.PeakFringeIntensity * Pulse;
	}

	if (bUseTimeDilation)
	{
		const float SlowMo = 1.f - FMath::SmoothStep(0.7f, 1.f, RewindAlpha);
		const float WorldDilation = FMath::Max(FMath::Lerp(1.f, Punch.PeakWorldTimeDilation, SlowMo), 0.05f);
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), WorldDilation);
		Character->CustomTimeDilation = 1.f / WorldDilation;
	}
}

void UChronoRewind::UpdateCameraFocus(float DeltaTime)
{
	if (!bCameraFocusActive || !LocalController) return;

	FVector ToFocus = FocusPoint - Character->GetActorLocation();
	ToFocus.Z = 0.f;

	// FocusYaw starts as the player's current Yaw, @see BeginPunch().
	// Don't override it unless the player is more than 100cm away from the blast.
	// This avoid unwanted weird camera movements where the camera might swing wildly.
	if (ToFocus.SizeSquared() > FMath::Square(100.f))
	{
		FocusYaw = ToFocus.Rotation().Yaw;
	}

	const FRotator Current = LocalController->GetControlRotation();
	const FRotator Target(Current.Pitch, FocusYaw, Current.Roll);
	LocalController->SetControlRotation(FMath::RInterpTo(Current, Target, DeltaTime, Punch.CameraTurnSpeed));
}

void UChronoRewind::UpdateGhosts(float DeltaTime, float RewindAlpha)
{
	if (!bPunchActive || !Ghosts || RewindAlpha <= 0.f || RewindAlpha >= 1.f) return;

	GhostAccumulator += DeltaTime;
	if (GhostAccumulator >= Punch.GhostInterval)
	{
		GhostAccumulator = FMath::Fmod(GhostAccumulator, Punch.GhostInterval);
		Ghosts->SpawnGhost();
	}
}

void UChronoRewind::EndPunch()
{
	if (!bPunchActive) return;
	
	bPunchActive = false;

	if (OverlayMID && Character)
	{
		Character->GetMesh()->SetOverlayMaterial(BaseOverlayMaterial);
		OverlayMID = nullptr;
	}

	if (bCameraFocusActive)
	{
		bCameraFocusActive = false;
		if (LocalController)
		{
			LocalController->SetIgnoreLookInput(false);
		}
	}

	if (Camera)
	{
		Camera->SetFieldOfView(BaseFOV);
		Camera->PostProcessSettings.SceneFringeIntensity = BaseFringeIntensity;
		Camera->PostProcessSettings.bOverride_SceneFringeIntensity = bBaseFringeOverride;
		Camera->PostProcessSettings.MotionBlurAmount = BaseMotionBlurIntensity;
		Camera->PostProcessSettings.bOverride_MotionBlurAmount = bBaseMotionBlurOverride;
	}

	if (bUseTimeDilation)
	{
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
		if (Character)
		{
			Character->CustomTimeDilation = 1.f;
		}
	}
}

void UChronoRewind::Finish()
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnChronoRewindFinished.Broadcast();
	}

	EndTask();
}
