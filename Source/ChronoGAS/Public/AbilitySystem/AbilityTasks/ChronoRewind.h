// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ChronoRewind.generated.h"

class ACharacter;
class AController;
class UCameraComponent;
class UTemporalBufferComponent;

DECLARE_MULTICAST_DELEGATE(FOnChronoRewindFinished);


/**
 * Camera and time effects that pulse during the rewind.
 * Peaks halfway through, in sync with the rewind EaseInOut.
 */
USTRUCT(BlueprintType)
struct FChronoRewindPunch
{
	GENERATED_BODY()
	
	// Extra camera FOV at the peak, in degrees. Will be added to the default camera FOV.
	UPROPERTY(EditAnywhere, Category = "Rewind Punch")
	float ExtraFOV = 22.f;

	// Chromatic aberration intensity at the peak (0 = off, 7 = max)
	UPROPERTY(EditAnywhere, Category = "Rewind Punch", meta = (ClampMin = "0.0", ClampMax = "7.0"))
	float PeakFringeIntensity = 4.f;

	// World speed at the peak, 1 = normal, 0.25 = quarter speed. The player stays at full speed.
	// Works in Standalone mode only.
	UPROPERTY(EditAnywhere, Category = "Rewind Punch", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float PeakWorldTimeDilation = 0.25f;

	// Adds motion blur
	UPROPERTY(EditAnywhere, Category = "Rewind Punch", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PeakMotionBlurAmount = 1.f;

	// Turn the camera to keep the blast in view during the rewind
	UPROPERTY(EditAnywhere, Category = "Rewind Punch")
	bool bFocusCameraOnBlast = true;
	
	// Speed of the camera turning to the blast.
	UPROPERTY(EditAnywhere, Category = "Rewind Punch", meta = (ClampMin = "0.0", EditCondition = "bFocusCameraOnBlast"))
	float CameraTurnSpeed = 8.f;

	// Pause on the blast point before the rewind starts, so the player can see the blast.
	// This value is not dilated by time.
	UPROPERTY(EditAnywhere, Category = "Rewind Punch", meta = (ClampMin = "0.0"))
	float HoldDuration = 0.3f;
};

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
		 * @param InPunch = Camera and time effects during rewind
		 * @return UChronoRewind = task instance
		 */
		static UChronoRewind* CreateChronoRewindTask(UGameplayAbility* OwningAbility, UTemporalBufferComponent* InBuffer, float InDuration, const FChronoRewindPunch& InPunch);

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

	// Captures the camera's starting values and enables the effects
	void BeginPunch();

	// Rewind Punch stays 0 during the hold till the beginning of rewind
	void ApplyPunch(float RewindAlpha);

	// Turns the camera towards the blast
	void UpdateCameraFocus(float DeltaTime);
	
	// Puts the camera and time back as they were.
	void EndPunch();

	void Finish();

	UPROPERTY()
	TObjectPtr<UTemporalBufferComponent> Buffer;

	UPROPERTY()
	TObjectPtr<ACharacter> Character;

	UPROPERTY()
	TObjectPtr<UCameraComponent> Camera;

	// Duration that the task will run - rewind will happen
	float Duration = 0.6f;

	float ElapsedTime = 0.f;

	bool bMovementTakenOver = false;

	FChronoRewindPunch Punch;

	float BaseFOV = 90.f;
	float BaseFringeIntensity = 0.f;
	float BaseMotionBlurIntensity = 0.5f;
	bool bBaseFringeOverride = false;
	bool bBaseMotionBlurOverride = false;

	bool bPunchActive = false;
	bool bUseTimeDilation = false;

	UPROPERTY()
	TObjectPtr<AController> LocalController;
	// Location of the blast
	FVector FocusPoint = FVector::ZeroVector;
	// Yaw where the camera going to look at
	double FocusYaw = 0.0;
	bool bCameraFocusActive = false;
	
};
