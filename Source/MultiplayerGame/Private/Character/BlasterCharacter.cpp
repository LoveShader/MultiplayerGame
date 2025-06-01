// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "NetworkReplayStreaming.h"
#include "BlasterComponents/CombatComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BlasterComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "MultiplayerGame/MultiplayerGame.h"
#include "Net/UnrealNetwork.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Weapon/Weapon.h"
#include "GameMode/BlasterGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "PlayerState/BlasterPlayerState.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	//here CameraBoom Attach to Mesh, for crouch will change capsule size
	//so there need attach to mesh
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>("FollowCamera");
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 850.0f, 0.0f);

	//Create Combat Component
	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	Combat->SetIsReplicated(true);

	//Set Crouch Enabled
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	//Set Capsule and Mesh collision sets
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);

	//set Turn in place state
	TurningInPlace = ETurningInPlace::ETIP_NoTurning;
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;

	DissolveTimeLine = CreateDefaultSubobject<UTimelineComponent>("DissolveTimeLineComponent");
}

void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(GetController()) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}
}

UCombatComponent* ABlasterCharacter::GetCombatComponentForUI()
{
	return Combat;
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	UpdateHUDHealth();

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}
	
	//Setup Input Mapping Context, add it to Subsystem
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		if (Subsystem)
		{
			Subsystem->AddMappingContext(InputContext, 0);
		}
	}
	
}

void ABlasterCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MoveVector = Value.Get<FVector2D>();

	if (Controller)
	{
		//Use Controller Get YawRotation
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation = FRotator(0.0f, Rotation.Yaw, 0.0f);

		//Get Forward Vector and Right Vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		//Add MovementInput for X direction and Y direction
		AddMovementInput(ForwardDirection, MoveVector.Y);
		AddMovementInput(RightDirection, MoveVector.X);
	}
}

void ABlasterCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookVector = Value.Get<FVector2D>();

	if (Controller)
	{
		AddControllerPitchInput(LookVector.Y);
		AddControllerYawInput(LookVector.X);
	}
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappedWeapon);
		}
		else
		{
			ServerEquipButtonPressed();	
		} 
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)
	{
		UnCrouch();
	} else
	{
		Crouch();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	} 
}

void ABlasterCharacter::AimButtonReleased()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	} 
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	//if don't equip weapon, then just return
	if (Combat && Combat->EquippedWeapon == nullptr)	return;

	//Get Character Velocity
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0f;
	float Speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();
	
	if (Speed == 0.0f && !bIsInAir)	// stand, not running and jumping
	{
		FRotator CurrentAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);;
		FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = Delta.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NoTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}

	if (Speed > 0.0f || bIsInAir)
	{
		StartingAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
		AO_Yaw = 0.0f;	//when running and jumping, need set AO_Yaw equal 0.0f
		TurningInPlace = ETurningInPlace::ETIP_NoTurning;
		bUseControllerRotationYaw = true;
	}
	AO_Pitch = GetBaseAimRotation().Pitch;

	//because Pitch is compress to [0, 360), there need convert this to [-90,0)
	if (AO_Pitch > 90.0f && !IsLocallyControlled())
	{
		FVector2D InRange = FVector2D(270.0f,360.0f);
		FVector2D OutRange = FVector2D(-90.0f, 0.0f);
		AO_Pitch =  FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::Jump()
{
	if (bIsCrouched)
	{
		UnCrouch();
	} else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->FireButtonPressed(false);
	}
}

void ABlasterCharacter::PlayFireMontage(bool bIsAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)	return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireMontage)
	{
		AnimInstance->Montage_Play(FireMontage);
		FName SectionName = bIsAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::Elim()
{
	NetMulticastElim();
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ABlasterCharacter::ElimTimerFinished,
		ElimDelay);
}

void ABlasterCharacter::NetMulticastElim_Implementation()
{
	bIsElimed = true;
	PlayElimMontage();
	// add dissolve material
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.0f);
	}
	StartDissolve();

	//Spawn Elim bot
	if (ElimBotParticle)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.0f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotParticle,
			ElimBotSpawnPoint,
			GetActorRotation()
		);
	}

	if (ElimBotSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}
	
	if (Combat)
	{
		Combat->DroppedWeapon();
		BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(GetController()) : BlasterPlayerController;
		if (BlasterPlayerController)
		{
			BlasterPlayerController->ClearWeaponAmmoHUD();
		}
	}

	//Disable character Movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if (BlasterPlayerController)
	{
		DisableInput(BlasterPlayerController);
	}

	//Disable Colliison
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABlasterCharacter::PlayHitReactMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && HitReactMontage && !AnimInstance->IsAnyMontagePlaying())
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName = FName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
                                      AController* InstigatedBy, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth);
	if (IsLocallyControlled())
		UpdateHUDHealth();
	PlayHitReactMontage();

	if (Health == 0.0f)
	{
		ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		if (BlasterGameMode)
		{
			BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(GetController()) : BlasterPlayerController;
			ABlasterPlayerController* AttackerController  = Cast<ABlasterPlayerController>(InstigatedBy);
			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
		}
	}
}

void ABlasterCharacter::OnRep_OverlappedWeapon(AWeapon* LastWeapon)
{
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
	
	if (OverlappedWeapon)
	{
		OverlappedWeapon->ShowPickupWidget(true);
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappedWeapon);
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.0f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	} else if (AO_Yaw < -90.0f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NoTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.0f, DeltaTime, 5.0f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.0f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NoTurning;
			StartingAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
		}
	}
}

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	//avoid other character can't see the character that owner by controller, so need this line
	if (!IsLocallyControlled())	return;
		
	if (((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold))
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->SetOwnerNoSee(true);
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->SetOwnerNoSee(false);
		}
	}
}

void ABlasterCharacter::OnRep_Health()
{
	//我们总是在服务器端进行开火受伤检测的，因此需要将health通知到客户端，由客户端进行血条更新
	if (IsLocallyControlled())
		UpdateHUDHealth();
	PlayHitReactMontage();
}

void ABlasterCharacter::ElimTimerFinished()
{
	ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if (BlasterGameMode)
	{
		BlasterGameMode->RequestRespawn(this, BlasterPlayerController);
	}
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue("Dissolve", DissolveValue);
	}
}

void ABlasterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeLine)
	{
		DissolveTimeLine->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeLine->Play();
	}
}

void ABlasterCharacter::PollInit()
{
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState)
		{
			BlasterPlayerState->AddScore(0.f);
			BlasterPlayerState->AddDefeats(0);
		}
	}
}

void ABlasterCharacter::SetOverlappedWeapon(AWeapon* Weapon)
{
	if (OverlappedWeapon)
	{
		OverlappedWeapon->ShowPickupWidget(false);
	}
	OverlappedWeapon = Weapon;
	if (IsLocallyControlled())
	{
		if (OverlappedWeapon)
		{
			OverlappedWeapon->ShowPickupWidget(true);
		}
	}
}

bool ABlasterCharacter::IsWeaponEquipped() const
{
	return Combat && Combat->EquippedWeapon;
}

bool ABlasterCharacter::IsAiming() const
{
	return Combat && Combat->bIsAiming;
}

AWeapon* ABlasterCharacter::GetEquippedWeapon() const
{
	if (Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (Combat == nullptr)	return FVector();
	return Combat->HitTarget;
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	PollInit();
	AimOffset(DeltaTime);
	HideCameraIfCharacterClose();
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Move);
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Look);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ABlasterCharacter::Jump);
		EnhancedInput->BindAction(EquipAction, ETriggerEvent::Started, this, &ABlasterCharacter::EquipButtonPressed);
		EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Started, this, &ABlasterCharacter::CrouchButtonPressed);
		EnhancedInput->BindAction(AimAction, ETriggerEvent::Started, this, &ABlasterCharacter::AimButtonPressed);
		EnhancedInput->BindAction(AimAction, ETriggerEvent::Completed, this, &ABlasterCharacter::AimButtonReleased);
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &ABlasterCharacter::FireButtonPressed);
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Completed, this, &ABlasterCharacter::FireButtonReleased);
	}
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappedWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat->Character = this;
	}
}
