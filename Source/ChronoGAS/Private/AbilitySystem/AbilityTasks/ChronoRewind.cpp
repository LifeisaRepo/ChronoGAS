// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.


#include "AbilitySystem/AbilityTasks/ChronoRewind.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "TemporalBufferComponent.h"
#include "Kismet/GameplayStatics.h"

UChronoRewind::UChronoRewind()
{
	bTickingTask = true;
}

UChronoRewind* UChronoRewind::CreateChronoRewindTask(UGameplayAbility* OwningAbility, UTemporalBufferComponent* InBuffer, float InDuration)
{
	UChronoRewind* Task = NewAbilityTask<UChronoRewind>(OwningAbility);
	Task->Buffer = InBuffer;
	Task->Duration = FMath::Max(InDuration, 0.01f);
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

}

void UChronoRewind::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	ElapsedTime += DeltaTime;

	ApplyRewindPose(FMath::Clamp(ElapsedTime / Duration, 0.f, 1.f));

	if (ElapsedTime < Duration)
	{
		return;
	}

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

void UChronoRewind::Finish()
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnChronoRewindFinished.Broadcast();
	}

	EndTask();
}
