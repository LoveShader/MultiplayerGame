// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "NetworkReplayStreaming.h"
#include "BlasterComponents/BuffComponent.h"
#include "BlasterComponents/CombatComponent.h"
#include "BlasterComponents/LagCompensationComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BlasterComponents/CombatComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "MultiplayerGame/MultiplayerGame.h"
#include "Net/UnrealNetwork.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Weapon/Weapon.h"
#include "GameMode/BlasterGameMode.h"
#include "HUD/OverheadWidget.h"
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

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("Buff"));
	Buff->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));

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

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>("AttachedGrenade");
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	/** 
	* Hit boxes for server-side rewind
	*/
	head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	head->SetupAttachment(GetMesh(), FName("head"));
	head->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	pelvis->SetupAttachment(GetMesh(), FName("pelvis"));
	pelvis->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
	spine_02->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
	spine_03->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	upperarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	upperarm_l->SetupAttachment(GetMesh(), FName("upperarm_l"));
	upperarm_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	upperarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	upperarm_r->SetupAttachment(GetMesh(), FName("upperarm_r"));
	upperarm_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	lowerarm_l->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	lowerarm_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	lowerarm_r->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	lowerarm_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName("hand_l"));
	hand_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName("hand_r"));
	hand_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	blanket = CreateDefaultSubobject<UBoxComponent>(TEXT("blanket"));
	blanket->SetupAttachment(GetMesh(), FName("backpack"));
	blanket->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	backpack = CreateDefaultSubobject<UBoxComponent>(TEXT("backpack"));
	backpack->SetupAttachment(GetMesh(), FName("backpack"));
	backpack->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	thigh_l = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	thigh_l->SetupAttachment(GetMesh(), FName("thigh_l"));
	thigh_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	thigh_r = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	thigh_r->SetupAttachment(GetMesh(), FName("thigh_r"));
	thigh_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	calf_l->SetupAttachment(GetMesh(), FName("calf_l"));
	calf_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	calf_r->SetupAttachment(GetMesh(), FName("calf_r"));
	calf_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName("foot_l"));
	foot_l->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName("foot_r"));
	foot_r->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(GetController()) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::UpdateHUDShield()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(GetController()) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->UpdateHUDShield(Shield, MaxShield);
	}
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if (BlasterGameMode)
	{
		if (Combat && Combat->EquippedWeapon && BlasterGameMode->GetMatchState() == MatchState::Cooldown)
		{
			Combat->EquippedWeapon->Destroy();		
		}
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
	UpdateHUDShield();
	SpawnDefaultWeapon();

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}
	
	//Setup Input Mapping Context, add it to Subsystem
	PollInitInput();

	//Hide The ThrowGrenade In BeginPlay
	AttachedGrenade->SetVisibility(false);
}

void ABlasterCharacter::Move(const FInputActionValue& Value)
{
	if (bDisableGameplay)
		return;
	
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
	if (bDisableGameplay)
		return;
	
	if (Combat)
	{
		if (OverlappedWeapon)
		{
			ServerEquipButtonPressed();
		} else if (Combat->CanSwapWeapon())
		{
			Combat->SwapWeapons();
		}
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay)
		return;
	
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
	if (bDisableGameplay)
		return;
	
	if (Combat)
	{
		Combat->SetAiming(true);
	} 
}

void ABlasterCharacter::AimButtonReleased()
{
	if (bDisableGameplay)
		return;
	
	if (Combat)
	{
		Combat->SetAiming(false);
	} 
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	
	//because Pitch is compress to [0, 360), there need convert this to [-90,0)
	if (AO_Pitch > 90.0f && !IsLocallyControlled())
	{
		FVector2D InRange = FVector2D(270.0f,360.0f);
		FVector2D OutRange = FVector2D(-90.0f, 0.0f);
		AO_Pitch =  FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

float ABlasterCharacter::CalculateSpeed() const
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0f;
	return Velocity.Size();
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	//if don't equip weapon, then just return
	if (Combat && Combat->EquippedWeapon == nullptr)	return;
	bRotateRootBone = true;

	//Get Character Velocity
	float Speed = CalculateSpeed();
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
	CalculateAO_Pitch();
}

void ABlasterCharacter::OnRep_ReplicateMovement()
{
	Super::OnRep_ReplicateMovement();
	
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.0f;
}

void ABlasterCharacter::SimProxiesTurn()
{
	if (Combat && Combat->EquippedWeapon == nullptr)	return;
	bRotateRootBone = true;
	//UE_LOG(LogTemp, Warning, TEXT("TurningInPlace is Not Turning"));
	float Speed = CalculateSpeed();

	if (Speed > 0.0f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NoTurning;
		return;
	}

	
	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	//UE_LOG(LogTemp, Warning, TEXT("ProxyYaw: %f"), ProxyYaw);
	if (FMath::Abs(ProxyYaw) > TurnThreshold) {
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
			//UE_LOG(LogTemp, Warning, TEXT("TurningInPlace is Right"));
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
			//UE_LOG(LogTemp, Warning, TEXT("TurningInPlace is Left"));
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NoTurning;
		}
		return;
	}

	TurningInPlace = ETurningInPlace::ETIP_NoTurning;
}

void ABlasterCharacter::Jump()
{
	if (bDisableGameplay)
		return;
	
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
	if (bDisableGameplay)
		return;
	
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (bDisableGameplay)
		return;
	
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->FireButtonPressed(false);
	}
}

void ABlasterCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay)
		return;
	
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->Reload();
	}
}

void ABlasterCharacter::ThrowGrenadeButtonPressed()
{
	if (bDisableGameplay)
	{
		return;
	}

	if (Combat)
	{
		Combat->ThrowGrenade();
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

void ABlasterCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)	return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName = Combat->EquippedWeapon->GetReloadMontageName();
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

bool ABlasterCharacter::CanReceiveAmmo(EWeaponType WeaponType) const
{
	return Combat && Combat->CanPickupAmmo(WeaponType);
}

void ABlasterCharacter::ReceiveAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (Combat == nullptr)	return;
	Combat->PickupAmmo(WeaponType, AmmoAmount);
}

void ABlasterCharacter::Elim()
{
	DropOrDestroyWeapons();
	NetMulticastElim();
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ABlasterCharacter::ElimTimerFinished,
		ElimDelay);
}

void ABlasterCharacter::ShowElimTextIfLocallyControlled()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(GetController()) : BlasterPlayerController;
	if (IsLocallyControlled() && BlasterPlayerController)
	{
		BlasterPlayerController->StartShowElimTextTimer();
	}
}

void ABlasterCharacter::DropOrDestroyWeapons()
{
	if (Combat == nullptr)
	{
		return;
	}

	Combat->DropOrDestroyWeapons();
}

void ABlasterCharacter::ClearWeaponTypeText()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(GetController()) : BlasterPlayerController;
	if (IsLocallyControlled() && BlasterPlayerController)
	{
		BlasterPlayerController->UpdateHUDWeaponType("");
	}
}

void ABlasterCharacter::ClearCarriedAmmoText()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(GetController()) : BlasterPlayerController;
	if (IsLocallyControlled() && BlasterPlayerController)
	{
		BlasterPlayerController->UpdateHUDCarriedAmmo(0);
	}
}

void ABlasterCharacter::NetMulticastElim_Implementation()
{
	bIsElimed = true;
	//Show Elim Text On Current Machine
	ShowElimTextIfLocallyControlled();
	ClearWeaponTypeText();
	ClearCarriedAmmoText();
	
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
	
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(GetController()) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->ClearWeaponAmmoHUD();
	}

	//Disable character Movement
	bDisableGameplay = true;
	GetCharacterMovement()->DisableMovement();

	//Disable Colliison
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bool bHideSniperScope = IsLocallyControlled() &&
		Combat &&
			IsAiming() &&
			Combat->EquippedWeapon &&
				Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;

	if (bHideSniperScope)
	{
		ShowSniperScopeWidget(false);
	}
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
	float DamageToHealth = Damage;
	if (Shield <= DamageToHealth)
	{
		DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.0f, Damage);
		Shield = 0.0f;
	}
	else
	{
		Shield = FMath::Clamp(Shield - DamageToHealth, 0.0f, MaxShield);
		DamageToHealth = 0.0f;
	}

	Health = FMath::Clamp(Health - DamageToHealth, 0.0f, MaxHealth);
	if (IsLocallyControlled())
	{
		UpdateHUDShield();
		UpdateHUDHealth();
	}
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

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (!Combat)	return ECombatState::ECS_MAX;
	return Combat->CombatState;
}

void ABlasterCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (!OverheadWidget) return;
	
	if (UUserWidget* UserWidget = OverheadWidget->GetUserWidgetObject())
	{
		if (APlayerState* PS = GetPlayerState())
		{
			if (UOverheadWidget* Overhead = Cast<UOverheadWidget>(UserWidget))
			{
				Overhead->SetDisplayText(PS->GetPlayerName());
			}
		}
	}
}

void ABlasterCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (LagCompensation)
	{
		LagCompensation->Character = this;
		LagCompensation->Controller = Cast<ABlasterPlayerController>(NewController);
	}

	if (!OverheadWidget) return;
	
	if (UUserWidget* UserWidget = OverheadWidget->GetUserWidgetObject())
	{
		if (APlayerState* PS = GetPlayerState())
		{
			if (UOverheadWidget* Overhead = Cast<UOverheadWidget>(UserWidget))
			{
				Overhead->SetDisplayText(PS->GetPlayerName());
			}
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

void ABlasterCharacter::OnRep_Health(float LastHealth)
{
	//我们总是在服务器端进行开火受伤检测的，因此需要将health通知到客户端，由客户端进行血条更新
	if (IsLocallyControlled())
	{
		UpdateHUDHealth();
	}
	if (Health < LastHealth)
	{
		PlayHitReactMontage();
	}
}

void ABlasterCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();
	if (Shield < LastShield)
	{
		PlayHitReactMontage();
	}
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

void ABlasterCharacter::PollInitInput()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		if (Subsystem)
		{
			Subsystem->AddMappingContext(InputContext, 0);
		}
		bInputsSet = true;
	}
}

void ABlasterCharacter::SpawnDefaultWeapon() const
{
	if (HasAuthority() && DefaultWeaponClass && !bIsElimed)
	{
		if (UWorld* World = GetWorld())
		{
			AWeapon* DefaultWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);

			if (Combat && DefaultWeapon)
			{
				Combat->EquipWeapon(DefaultWeapon);
				DefaultWeapon->SetWeaponoDestroyed(true);
			}
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

bool ABlasterCharacter::IsLocallyReloading() const
{
	return Combat && Combat->GetLocallyReloading();
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
	if (!bInputsSet && HasAuthority())
		PollInitInput();

	if (GetLocalRole() > ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicateMovement();
		}
		CalculateAO_Pitch();
	}
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
		EnhancedInput->BindAction(ReloadAction, ETriggerEvent::Started, this, &ABlasterCharacter::ReloadButtonPressed);
		EnhancedInput->BindAction(ThrowGrenadeAction, ETriggerEvent::Started, this, &ABlasterCharacter::ThrowGrenadeButtonPressed);
	}
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappedWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME_CONDITION(ABlasterCharacter, Shield, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat->Character = this;
	}

	if (Buff)
	{
		Buff->Character = this;
		Buff->SetInitialSpeeds(
			GetCharacterMovement()->MaxWalkSpeed,
			GetCharacterMovement()->MaxWalkSpeedCrouched
		);
		Buff->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
	}

	if (LagCompensation)
	{
		LagCompensation->Character = this;
		if (Controller)
		{
			LagCompensation->Controller = Cast<ABlasterPlayerController>(Controller);
		}
	}
}
