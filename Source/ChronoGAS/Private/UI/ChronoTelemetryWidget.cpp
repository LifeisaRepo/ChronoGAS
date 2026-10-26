// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.


#include "UI/ChronoTelemetryWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/ChronoAttributeSet.h"
#include "AbilitySystem/ChronoGameplayTags.h"
#include "GameFramework/Character.h"
#include "TemporalBufferComponent.h"

void UChronoTelemetryWidget::SetObservedCharacter(ACharacter* InCharacter)
{
	Buffer = InCharacter ? InCharacter->FindComponentByClass<UTemporalBufferComponent>() : nullptr;
	ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InCharacter);
	RefreshText();
}

void UChronoTelemetryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateHealth(InDeltaTime);

	if (!bUpdateTelemetry) return;

	RefreshAccumulator += InDeltaTime;
	if (RefreshAccumulator < RefreshInterval) return;
	
	RefreshAccumulator = 0.f;

	RefreshText();
}

void UChronoTelemetryWidget::RefreshText()
{
	if (!Buffer.IsValid() || !ASC.IsValid()) return;

	const UTemporalBufferComponent* TemporalBuffer = Buffer.Get();
	const int32 Count = TemporalBuffer->GetNumSnapshots();

	double SpanSeconds = 0.0;
	float HealIfUsed = 0.f;

	if (Count > 0)
	{
		const FTemporalSnapshot& Newest = TemporalBuffer->GetSnapshotByAge(0);
		const FTemporalSnapshot& Oldest = TemporalBuffer->GetSnapshotByAge(Count - 1);
		SpanSeconds = Newest.Timestamp - Oldest.Timestamp;

		const float HealthNow = ASC->GetNumericAttribute(UChronoAttributeSet::GetHealthAttribute());
		HealIfUsed = FMath::Max(0.f, Oldest.Health - HealthNow);
	}

	const bool bRewinding = ASC->HasMatchingGameplayTag(TAG_State_Rewinding);

	float CooldownRemaining = 0.f;
	const FGameplayEffectQuery CooldownQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(TAG_Cooldown_ChronoRecall));
	for (const float Remaining : ASC->GetActiveEffectsTimeRemaining(CooldownQuery))
	{
		CooldownRemaining = FMath::Max(CooldownRemaining, Remaining);
	}

	FString AbilityState = TEXT("Ready");
	if (bRewinding)
	{
		AbilityState = TEXT("REWINDING");
	}
	else if (CooldownRemaining > 0.f)
	{
		AbilityState = FString::Printf(TEXT("Cooldown %.1f s"), CooldownRemaining);
	}

	const ENetMode NetMode = GetWorld()->GetNetMode();
	const TCHAR* NetRole = NetMode == NM_Standalone ? TEXT("Standalone") : NetMode == NM_Client ? TEXT("Client") : TEXT("Server");

	OutTelemetryText = FText::FromString(FString::Printf(
		TEXT("CHRONO RECALL TELEMETRY\n")
		TEXT("Buffer: %d/%d | %.2f s\n")
		TEXT("Path distance: %.0f cm\n")
		TEXT("Heal if used: +%.0f HP\n")
		TEXT("Ability: %s\n")
		TEXT("Network role: %s"),
		Count, TemporalBuffer->GetCapacity(), SpanSeconds,
		TemporalBuffer->ComputePathDistance(), HealIfUsed,
		*AbilityState, NetRole));
}

void UChronoTelemetryWidget::UpdateHealth(float DeltaTime)
{
	if (!ASC.IsValid()) return;
	
	const float Health = ASC->GetNumericAttribute(UChronoAttributeSet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(UChronoAttributeSet::GetMaxHealthAttribute());
	const float TargetPercent = MaxHealth > 0.f ? Health / MaxHealth : 0.f;

	OutHealthPercent = FMath::FInterpTo(OutHealthPercent, TargetPercent, DeltaTime, 8.f);
	OutHealthText = FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Health, MaxHealth));

}
