// Copyright (c) 2026 Sanjyot Dahale. All rights reserved.


#include "AbilitySystem/ChronoAbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"

void UChronoAbilitySystemComponent::GrantStartupAbilities(const TArray<TSubclassOf<UGameplayAbility>>& AbilitiesToGrant)
{
	for (const TSubclassOf<UGameplayAbility> AbilityClass : AbilitiesToGrant)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		GiveAbility(AbilitySpec);
	}
	bStartupAbilitiesGranted = true;
	AbilitiesGivenDelegate.Broadcast();
}

// Will trigger in clients once abilities are given
void UChronoAbilitySystemComponent::OnRep_ActivateAbilities()
{
	Super::OnRep_ActivateAbilities();

	if (!bStartupAbilitiesGranted)
	{
		bStartupAbilitiesGranted = true;
		AbilitiesGivenDelegate.Broadcast();
	}
}
