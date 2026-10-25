// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.


#include "AbilitySystem/GA_ChronoRecall.h"
#include "AbilitySystem/AbilityTasks/ChronoRewind.h"
#include "AbilitySystem/ChronoGameplayTags.h"
#include "TemporalBufferComponent.h"
#include "GameFramework/Actor.h"

UGA_ChronoRecall::UGA_ChronoRecall()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	AbilityTags.AddTag(TAG_Ability_ChronoRecall);
}

void UGA_ChronoRecall::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const AActor* Avatar = GetAvatarActorFromActorInfo();
	
	TemporalBuffer = Avatar ? Avatar->FindComponentByClass<UTemporalBufferComponent>() : nullptr;
	if (!TemporalBuffer)
	{
		UE_LOG(LogTemp, Warning, TEXT("ChronoRecall: %s has no TemporalBufferComponent"), *GetNameSafe(Avatar));
		// End everywhere immediately.
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("ChronoRecall [%s, %s]: rewind started"), *Avatar->GetName(), *UEnum::GetValueAsString(Avatar->GetLocalRole()));

	UChronoRewind* RewindTask = UChronoRewind::CreateChronoRewindTask(this, TemporalBuffer, RewindDuration);
	RewindTask->OnChronoRewindFinished.AddUObject(this, &UGA_ChronoRecall::OnRewindFinished);
	RewindTask->ReadyForActivation();
}

void UGA_ChronoRecall::OnRewindFinished()
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	UE_LOG(LogTemp, Display, TEXT("ChronoRecall [%s, %s]: rewind finished"), *Avatar->GetName(), *UEnum::GetValueAsString(Avatar->GetLocalRole()));
	
	if (TemporalBuffer)
	{
		TemporalBuffer->ResetBuffer();
	}

	// bReplicateEndAbility will be false. Because we want both client and server to end
	// abilities when their own rewinds actually finish.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);

}
