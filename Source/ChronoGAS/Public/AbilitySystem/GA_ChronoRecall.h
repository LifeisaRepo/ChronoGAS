// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_ChronoRecall.generated.h"

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

	// Called by the rewind task once it has finished a rewind run.
	void OnRewindFinished();

	// Duration for which the rewind effect runs, in seconds
	UPROPERTY(EditDefaultsOnly, Category="ChronoRecall")
	float RewindDuration = 0.6f;

	// Avatar's buffer, found on activation
	UPROPERTY()
	TObjectPtr<UTemporalBufferComponent> TemporalBuffer;
};
