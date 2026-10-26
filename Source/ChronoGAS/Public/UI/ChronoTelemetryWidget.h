// Copyright (c) 2026 Sanjyot Dahale. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChronoTelemetryWidget.generated.h"

class ACharacter;
class UAbilitySystemComponent;
class UTemporalBufferComponent;

// DECLARE_DYNAMIC_DELEGATE(FShouldUpdateText);

/**
 * 
 */
UCLASS()
class CHRONOGAS_API UChronoTelemetryWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// Character whose numbers are shown
	UFUNCTION(BlueprintCallable)
	void SetObservedCharacter(ACharacter* InCharacter);

	// Feed this to a Text block to see the telemetry stats
	UPROPERTY(BlueprintReadOnly, Category = "ChronoTelemetryWidget")
	FText OutTelemetryText;

	UPROPERTY(BlueprintReadOnly, Category = "ChronoTelemetryWidget")
	float OutHealthPercent = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "ChronoTelemetryWidget")
	FText OutHealthText;

	// Disable the telemetry when only using Health bars
	UPROPERTY(EditDefaultsOnly)
	bool bUpdateTelemetry = true;

protected:

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	void RefreshText();

	void UpdateHealth(float DeltaTime);

	// This widget will be owned by the PlayerController, which may outlive the character, hence weak pointers
	TWeakObjectPtr<UTemporalBufferComponent> Buffer;

	TWeakObjectPtr<UAbilitySystemComponent> ASC;

	float RefreshAccumulator = 0.f;
	const float RefreshInterval = 0.1f;	// ~10 times per second 
	
};
