// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_ChronoRecall.generated.h"

class UGameplayEffect;
class UTemporalBufferComponent;

/**
 * Chrono Recall = Rewinds the owner along its recorded path
 */
UCLASS()
class CHRONOGAS_API UGA_ChronoRecall : public UGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_ChronoRecall();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
private:

	// Triggers a blast at the point where the rewind starts (newest snapshot)
	void Detonate();

	// Called by the rewind task once it has finished a rewind run.
	void OnRewindFinished();

	// Server only. Gives back the health that was lost since the oldest snapshot
	void ApplyHeal();

	// Duration for which the rewind effect runs, in seconds
	UPROPERTY(EditDefaultsOnly, Category="ChronoRecall")
	float RewindDuration = 0.6f;

	// The instant GE which heals the owner at the end of rewind.
	UPROPERTY(EditDefaultsOnly, Category="ChronoRecall")
	TSubclassOf<UGameplayEffect> HealEffect;

	UPROPERTY(EditDefaultsOnly, Category = "ChronoRecall|Detonation")
	TSubclassOf<UGameplayEffect> BlastDamageEffect;

	// Length of recorded path at which blast reaches full power
	UPROPERTY(EditDefaultsOnly, Category="ChronoRecall|Detonation")
	float DistanceForFullPower = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "ChronoRecall|Detonation")
	float MinBlastRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "ChronoRecall|Detonation")
	float MaxBlastRadius = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "ChronoRecall|Detonation")
	float MinBlastDamage = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "ChronoRecall|Detonation")
	float MaxBlastDamage = 50.f;

	// Launch speed for characters at the center of the blast
	UPROPERTY(EditDefaultsOnly, Category = "ChronoRecall|Detonation")
	float MinLaunchSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "ChronoRecall|Detonation")
	float MaxLaunchSpeed = 1200.f;

	// Avatar's buffer, found on activation
	UPROPERTY()
	TObjectPtr<UTemporalBufferComponent> TemporalBuffer;
};
