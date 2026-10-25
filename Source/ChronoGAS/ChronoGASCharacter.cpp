// Copyright (c) 2026 Sanjyot Dahale. All rights reserved.

#include "ChronoGASCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystem/ChronoAbilitySystemComponent.h"
#include "AbilitySystem/ChronoAttributeSet.h"
#include "AbilitySystem/ChronoGameplayTags.h"
#include "TemporalBufferComponent.h"

DEFINE_LOG_CATEGORY(LogChronoCharacter);

//////////////////////////////////////////////////////////////////////////
// AChronoGASCharacter

AChronoGASCharacter::AChronoGASCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	AbilitySystemComponent = CreateDefaultSubobject<UChronoAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UChronoAttributeSet>("AttributeSet");

	TemporalBuffer = CreateDefaultSubobject<UTemporalBufferComponent>("TemporalBuffer");
}

void AChronoGASCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	//For SERVER
	InitAbilityActorInfo(TEXT("PossessedBy"));
}

void AChronoGASCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	//For Owning Client
	InitAbilityActorInfo(TEXT("OnRep_Controller"));
}

void AChronoGASCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	InitAbilityActorInfo(TEXT("BeginPlay"));

	// Do only once on SERVER
	if (HasAuthority())
	{
		InitializeDefaultAttributes();
		GrantStartupAbilities();
	}

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

UAbilitySystemComponent* AChronoGASCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAttributeSet* AChronoGASCharacter::GetAttributeSet() const
{
	return AttributeSet;
}

void AChronoGASCharacter::InitAbilityActorInfo(const FString& CallerName)
{
	// Player state does not need to outlive the pawn for this demo
	// So we are not using a dedicated PlayerState class
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	OnAbilityActorInfoSet.Broadcast(AbilitySystemComponent);


	FString Message = FString(TEXT("Caller Name = "));
	Message.Append(CallerName);
	Message.Append(FString(TEXT(" || Pawn = ")));
	Message.Append(GetName());
	Message.Append(FString(TEXT(" || Role = ")));
	Message.Append(UEnum::GetValueAsString(GetLocalRole()));
	Message.Append(FString(TEXT(" || PC ")));
	Message.Append(AbilitySystemComponent->AbilityActorInfo->PlayerController.IsValid() ? FString(TEXT("Valid")) : FString(TEXT("Not Valid")));
	Message.Append(FString(TEXT(" || Local ")));
	Message.Append(AbilitySystemComponent->AbilityActorInfo->IsLocallyControlled() ? FString(TEXT("Yes")) : FString(TEXT("No")));
	// UKismetSystemLibrary::PrintString(this, Message, true, true, FLinearColor::Blue, 10.f);
}

void AChronoGASCharacter::GrantStartupAbilities()
{
	// ONLY SERVER operation
	if (!HasAuthority()) return;

	AbilitySystemComponent->GrantStartupAbilities(StartupAbilities);
	
}

void AChronoGASCharacter::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level) const
{
	check(IsValid(AbilitySystemComponent));

	if(!ensureMsgf(GameplayEffectClass, TEXT("GameplayEffectClass is not valid. Please check `DefaultAttributes` is populated in blueprint.")))
	{
		return;	
	}

	FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	const FGameplayEffectSpecHandle EffectSpec = AbilitySystemComponent->MakeOutgoingSpec(GameplayEffectClass, Level, ContextHandle);
	
	if (!EffectSpec.IsValid()) return;	
	// I know I could have just used ApplyGameEffectSpecToSelf
	AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*EffectSpec.Data.Get(), AbilitySystemComponent);
}

void AChronoGASCharacter::InitializeDefaultAttributes() const
{
	// Initialize Health and Stamina at start-up
	ApplyEffectToSelf(DefaultAttributes);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AChronoGASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AChronoGASCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AChronoGASCharacter::Look);

		// Chrono Recall
		EnhancedInputComponent->BindAction(ChronoRecallAction, ETriggerEvent::Started, this, &AChronoGASCharacter::OnChronoRecallPressed);
	}
	else
	{
		UE_LOG(LogChronoCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AChronoGASCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AChronoGASCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AChronoGASCharacter::OnChronoRecallPressed()
{
	if (!AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_ChronoRecall)))
	{
		UE_LOG(LogChronoCharacter, Warning, TEXT("Chrono Recall did not activate. Might be already running - or some error -_-"));
	}
}
