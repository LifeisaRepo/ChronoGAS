// Copyright (c) 2026 Sanjyot Dahale. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ChronoAbilitySystemComponent.generated.h"

struct FGameplayTag;

DECLARE_MULTICAST_DELEGATE(FAbilitiesGiven);

/**
 * 
 */
UCLASS()
class CHRONOGAS_API UChronoAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:

	// UPROPERTY(BlueprintAssignable)
	FAbilitiesGiven AbilitiesGivenDelegate;

	UFUNCTION(BlueprintCallable)
	void GrantStartupAbilities(const TArray<TSubclassOf<UGameplayAbility>>& AbilitiesToGrant);

protected:
	UPROPERTY(BlueprintReadOnly)
	bool bStartupAbilitiesGranted = false;

	virtual void OnRep_ActivateAbilities() override;	
};
