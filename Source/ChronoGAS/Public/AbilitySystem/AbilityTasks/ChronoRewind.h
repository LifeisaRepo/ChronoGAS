// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ChronoRewind.generated.h"

class ACharacter;
class UTemporalBufferComponent;

DECLARE_MULTICAST_DELEGATE(FOnChronoRewindFinished);

/**
 * Rewinds the owner along its recorded path in a fixed duration
 */
UCLASS()
class CHRONOGAS_API UChronoRewind : public UAbilityTask
{
	GENERATED_BODY()

public:
		UChronoRewind();

		/**
		 * @brief Creates the rewind task
		 * @param OwningAbility = owning ability (GA_ChronoRecall)
		 * @param InBuffer = The temporal ring buffer holding the avatar's snapshots
		 * @param InDuration = Duration of the rewind effect. InDuration >= 0.01f
		 * @return UChronoRewind = task instance
		 */
		static UChronoRewind* CreateChronoRewindTask(UGameplayAbility* OwningAbility, UTemporalBufferComponent* InBuffer, float InDuration);

		FOnChronoRewindFinished OnChronoRewindFinished;

		virtual void Activate() override;

		virtual void TickTask(float DeltaTime) override;

protected:

	virtual void OnDestroy(bool bInOwnerFinished) override;

private:

	// Owner CMC is switched to Flying mode while the ability takes over
	void TakeOverMovement();

	// Undo TakOverMovement()
	void ReleaseMovement();

	// Places the character on the recorded path.
	// Alpha 0 = newest snapshot, 1 = oldest
	void ApplyRewindPose(float Alpha);

	void Finish();

	UPROPERTY()
	TObjectPtr<UTemporalBufferComponent> Buffer;

	UPROPERTY()
	TObjectPtr<ACharacter> Character;

	// Duration that the task will run - rewind will happen
	float Duration = 0.6f;

	float ElapsedTime = 0.f;

	bool bMovementTakenOver = false;
	
};
