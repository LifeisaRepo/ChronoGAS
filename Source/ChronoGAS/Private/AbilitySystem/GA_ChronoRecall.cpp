// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.


#include "AbilitySystem/GA_ChronoRecall.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/AbilityTasks/ChronoRewind.h"
#include "AbilitySystem/ChronoAttributeSet.h"
#include "AbilitySystem/ChronoGameplayTags.h"
#include "DrawDebugHelpers.h"
#include "TemporalBufferComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"

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

	// Detonate before rewind
	Detonate();

	UChronoRewind* RewindTask = UChronoRewind::CreateChronoRewindTask(this, TemporalBuffer, RewindDuration);
	RewindTask->OnChronoRewindFinished.AddUObject(this, &UGA_ChronoRecall::OnRewindFinished);
	RewindTask->ReadyForActivation();
}

void UGA_ChronoRecall::Detonate()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();

	if (!Avatar || !World || !TemporalBuffer) return;

	const float PathDistance = TemporalBuffer->ComputePathDistance();
	const float Power = FMath::Clamp(PathDistance / FMath::Max(DistanceForFullPower, 1.f), 0.f, 1.f);

	const FVector Origin = Avatar->GetActorLocation();
	const float Radius = FMath::Lerp(MinBlastRadius, MaxBlastRadius, Power);

	// Visual
	if (IsLocallyControlled())
	{
		const FColor Color = FLinearColor::LerpUsingHSV(FLinearColor::Green, FLinearColor::Red, Power).ToFColor(true);
		DrawDebugSphere(World, Origin, Radius, 24, Color, false, 5.f, 0, 2.f);
	}

	// Gameplay - only server

	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	//~ Overlap Sphere
	const float Damage = FMath::Lerp(MinBlastDamage, MaxBlastDamage, Power);
	const float LaunchSpeed = FMath::Lerp(MinLaunchSpeed, MaxLaunchSpeed, Power);

	const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = { UEngineTypes::ConvertToObjectType(ECC_Pawn) };

	const TArray<AActor*> ActorsToIgnore = { Avatar };

	TArray<AActor*> HitActors;

	UKismetSystemLibrary::SphereOverlapActors(Avatar, Origin, Radius, ObjectTypes, nullptr, ActorsToIgnore, HitActors);

	UE_LOG(LogTemp, Display, TEXT("ChronoRecall detonation: path %.0f cm, power %.2f, radius %.0f, damage %.0f, launch %.0f, %d hit"), PathDistance, Power, Radius, Damage, LaunchSpeed, HitActors.Num());

	//~ Create damage spec
	FGameplayEffectSpecHandle DamageSpec;
	if (ensureMsgf(BlastDamageEffect, TEXT("GA_ChronoRecall: BlastDamageEffect is not set. Set it in the ability Blueprint")))
	{
		DamageSpec = MakeOutgoingGameplayEffectSpec(BlastDamageEffect, GetAbilityLevel());
		if (DamageSpec.IsValid())
		{
			DamageSpec.Data->SetSetByCallerMagnitude(TAG_Data_DamageAmount, Damage);
		}
	}

	//~ Apply damage and knockback

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

	for (AActor* HitActor : HitActors)
	{
		// Damage anything with an ASC
		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
		if (SourceASC && TargetASC && DamageSpec.IsValid())
		{
			SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
			UE_LOG(LogTemp, Display, TEXT(" hit %s: Health now %.0f"), *HitActor->GetName(), TargetASC->GetNumericAttribute(UChronoAttributeSet::GetHealthAttribute()));
		}

		// Knockback: away and upward. 
		if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
		{
			const FVector Away = (HitCharacter->GetActorLocation() - Origin).GetSafeNormal2D();
			const float DistanceRatio = FMath::Clamp(FVector::Dist(HitCharacter->GetActorLocation(), Origin) / Radius, 0.f, 1.f);
			const float Falloff = 1.f - 0.6f * DistanceRatio;
			const FVector LaunchVelocity = (Away + FVector(0.f, 0.f, 0.6f)) * LaunchSpeed * Falloff;

			HitCharacter->LaunchCharacter(LaunchVelocity, true, true);
		}
	}

}

void UGA_ChronoRecall::OnRewindFinished()
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	UE_LOG(LogTemp, Display, TEXT("ChronoRecall [%s, %s]: rewind finished"), *Avatar->GetName(), *UEnum::GetValueAsString(Avatar->GetLocalRole()));

	// Restore health as per oldest snapshot. ONLY SERVER.
	if (HasAuthority(&CurrentActivationInfo))
	{
		ApplyHeal();
	}
	
	if (TemporalBuffer)
	{
		TemporalBuffer->ResetBuffer();
	}

	// bReplicateEndAbility will be false. Because we want both client and server to end
	// abilities when their own rewinds actually finish.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);

}

void UGA_ChronoRecall::ApplyHeal()
{
	if (!ensureMsgf(HealEffect, TEXT("GA_ChronoRecall: HealEffect is not set. Set it in ability's blueprint")))
	{
		return;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !TemporalBuffer || TemporalBuffer->GetNumSnapshots() == 0)
	{
		return;
	}

	const float HealthThen = TemporalBuffer->GetSnapshotByAge(TemporalBuffer->GetNumSnapshots() - 1).Health;
	const float HealthNow = ASC->GetNumericAttribute(UChronoAttributeSet::GetHealthAttribute());
	const float HealAmount = FMath::Max( 0.f, HealthThen - HealthNow);

	UE_LOG(LogTemp, Display, TEXT("ChronoRecall: Health at oldest snapshot = %.0f, now = %.0f, healing %.0f"), HealthThen, HealthNow, HealAmount);

	if (HealAmount <= 0.f)
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(HealEffect, GetAbilityLevel());
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_HealAmount, HealAmount);
	ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
}
