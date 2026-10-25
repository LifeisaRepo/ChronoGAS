// Copyright (c) 2026 Sanjyot Dahale. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TemporalBufferComponent.generated.h"

class UAbilitySystemComponent;

struct FTemporalSnapshot
{
	// World time when this snapshot was recorded
	double Timestamp = 0.0;

	// location and rotation of the owner
	FTransform Transform = FTransform::Identity;

	// velocity of the owner
	FVector Velocity = FVector::ZeroVector;

	// health of the owner
	float Health = 0.0f;

	// running number given by the buffer on every write
	int32 SampleIndex = INDEX_NONE;
};


/**
 * Holds temporal snapshots for the Chrono Recall ability in a custom ring-buffer
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHRONOGAS_API UTemporalBufferComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTemporalBufferComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// How many slots hold data
	int32 GetNumSnapshots() const { return Count; }

	// 0 = newest, GetNumSnapshots() - 1 = oldest
	const FTemporalSnapshot& GetSnapshotByAge(int32 Age) const;

	void SetRecordingPaused(bool bPaused);

	void ResetBuffer();

	// Total length of the recorded path.
	float ComputePathDistance() const;

protected:
	virtual void BeginPlay() override;	

private:

	bool ShouldRecord() const;

	bool ShouldDrawPath() const;

	// Captures owner's current state and writes it into the buffer
	void RecordSnapshot();

	// Copies snapshot into the buffer
	void WriteSnapshot(const FTemporalSnapshot& Snapshot);

	// Draws recorded path for one frame
	void DrawDebugPath() const;


	// Max Duration of recording
	UPROPERTY(EditDefaultsOnly, Category = "Temporal")
	float HistoryDuration = 3.0f;

	// Interval between each sample recording. ~20 samples per second
	UPROPERTY(EditDefaultsOnly, Category = "Temporal")
	float SampleInterval = 0.05f;

	// Speed in cm/s at which debug trail will be fully green
	UPROPERTY(EditDefaultsOnly, Category = "Temporal|Debug")
	float SpeedForGreen = 500.f;

	// Speed in cm / s at which debug trail will be fully red
	UPROPERTY(EditDefaultsOnly, Category = "Temporal|Debug")
	float SpeedForRed = 1500.f;

	// Draw a marker on every Nth snapshot
	UPROPERTY(EditDefaultsOnly, Category = "Temporal|Debug")
	int32 MarkerInterval = 10;

	// Half-size of marker box
	UPROPERTY(EditDefaultsOnly, Category = "Temporal|Debug")
	float MarkerSize = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "Temporal|Debug")
	float TrailThickness = 3.f;

	// Ring buffer
	TArray<FTemporalSnapshot> Snapshots;

	// Max capacity of the ring buffer
	int32 Capacity = 0;

	// Index to the slot where the next snapshot can be recorded
	int32 HeadIndex = 0;

	// Num of slots already allocated in the buffer.
	// Will become = Capcity once all the slots are filled.
	int32 Count = 0;

	// Just a sample counter
	int32 NextSampleIndex = 0;

	// Time collected since last sample was recorded
	float SampleAccumulator = 0.f;

	bool bRecordingPaused = false;

	// Owner's ASC
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedASC;
		
};
