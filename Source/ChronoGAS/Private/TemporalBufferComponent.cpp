// Copyright (c) 2026 Sanjyot Dahale. All rights reserved.


#include "TemporalBufferComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/ChronoAttributeSet.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<bool> CVarChronoDrawPath(
	TEXT("Chrono.DrawPath"),
	true,
	TEXT("Draw the temporal buffer's recorded path. 0 = off, 1 = on"));

UTemporalBufferComponent::UTemporalBufferComponent()
{

	PrimaryComponentTick.bCanEverTick = true;
	// CMC works in PrePhysics, so we record in PostPhysics, to get the correct values.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}


void UTemporalBufferComponent::BeginPlay()
{
	Super::BeginPlay();	

	// SampleInterval > 0
	SampleInterval = FMath::Max(SampleInterval, 0.01f);

	// Initialize buffer, should atleast 2 elements in the buffer
	Capacity = FMath::Max(2, FMath::RoundToInt(HistoryDuration / SampleInterval));
	Snapshots.SetNum(Capacity);

	CachedASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());	
	checkf(IsValid(CachedASC), TEXT("CachedASC cannot be null!"));
}

void UTemporalBufferComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ShouldRecord())
	{
		SampleAccumulator += DeltaTime;
		if (SampleAccumulator >= SampleInterval)
		{
			RecordSnapshot();

			// FMath::Fmod(A, B) = what's left of A after removing as many whole B's as possible
			SampleAccumulator = FMath::Fmod(SampleAccumulator, SampleInterval);
		}
	}	

	if (ShouldDrawPath())
	{
		DrawDebugPath();
	}
}

void UTemporalBufferComponent::SetRecordingPaused(bool bPaused)
{
	// Take one last snapshot when pausing, so the history ends exactly where the owner is right now
	if (bPaused && !bRecordingPaused)
	{
		RecordSnapshot();
	}

	bRecordingPaused = bPaused;

	// Start recording time from 0 again when recording resumes
	SampleAccumulator = 0.0f;
}

bool UTemporalBufferComponent::ShouldRecord() const
{
	if (bRecordingPaused)
	{
		return false;
	}

	// Only allow recording if Server or Owning client
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && (OwnerPawn->HasAuthority() || OwnerPawn->IsLocallyControlled());
}

bool UTemporalBufferComponent::ShouldDrawPath() const
{
	// Draw if CVar enabled
	if (!CVarChronoDrawPath.GetValueOnGameThread())
	{
		return false;
	}

	// Draw only on Owning client
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled() && OwnerPawn->IsPlayerControlled();
}

void UTemporalBufferComponent::RecordSnapshot()
{
	const AActor* OwnerActor = GetOwner();

	FTemporalSnapshot Snapshot;
	Snapshot.Timestamp = GetWorld()->GetTimeSeconds();
	Snapshot.Transform = OwnerActor->GetActorTransform();
	Snapshot.Velocity = OwnerActor->GetVelocity();
	Snapshot.Health = CachedASC ? CachedASC->GetNumericAttribute(UChronoAttributeSet::GetHealthAttribute()) : 0.f;

	WriteSnapshot(Snapshot);	
}

void UTemporalBufferComponent::WriteSnapshot(const FTemporalSnapshot& Snapshot)
{
	FTemporalSnapshot& Slot = Snapshots[HeadIndex];
	Slot = Snapshot;
	Slot.SampleIndex = NextSampleIndex++;

	HeadIndex = (HeadIndex + 1) % Capacity;
	Count = FMath::Min(Count + 1, Capacity);
}

void UTemporalBufferComponent::ResetBuffer()
{
	HeadIndex = 0;
	Count = 0;
	SampleAccumulator = 0.f;
}

float UTemporalBufferComponent::ComputePathDistance() const
{
	double Distance = 0.0;
	for (int32 Age = 0; Age + 1 < Count; Age++)
	{
		Distance += FVector::Dist(GetSnapshotByAge(Age).Transform.GetLocation(), GetSnapshotByAge(Age + 1).Transform.GetLocation());
	}

	return static_cast<float>(Distance);
}

const FTemporalSnapshot& UTemporalBufferComponent::GetSnapshotByAge(int32 Age) const
{
	checkf((Age >= 0 && Age < Count), TEXT("Age cannot be greater than or equal to the count of recorded snapshots"));
	int32 Index = (HeadIndex - 1 - Age + Capacity) % Capacity;
	return Snapshots[Index];
}

void UTemporalBufferComponent::DrawDebugPath() const
{
// Avoid calculations in Shipping, where debug drawing is disabled
#if ENABLE_DRAW_DEBUG
	const UWorld* World = GetWorld();
	const float SpeedRange = FMath::Max(SpeedForRed - SpeedForGreen, 1.f);

	for (int32 Age = 0; Age < Count; Age++)
	{
		const FTemporalSnapshot& Snapshot = GetSnapshotByAge(Age);
		const FVector Location = Snapshot.Transform.GetLocation();

		// Line from this snapshot to the next older one
		// Colors defined by how fast the character was moving
		if (Age + 1 < Count)
		{
			const FTemporalSnapshot& Older = GetSnapshotByAge(Age + 1);
			const float Speed = static_cast<float>(0.5 * (Snapshot.Velocity.Size() + Older.Velocity.Size()));
			const float Heat = FMath::Clamp((Speed - SpeedForGreen) / SpeedRange, 0.f, 1.f);
			const FColor Color = FLinearColor::LerpUsingHSV(FLinearColor::Green, FLinearColor::Red, Heat).ToFColor(true);

			DrawDebugLine(World, Location, Older.Transform.GetLocation(), Color, false, -1.f, 0, TrailThickness);
		}

		if (Snapshot.SampleIndex % MarkerInterval == 0)
		{
			DrawDebugBox(World, Location, FVector(MarkerSize), FColor::Cyan, false, -1.f, 0, TrailThickness);
		}
	}
#endif
}

