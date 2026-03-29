// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterComponents/CombatComponent.h"

#include "Camera/CameraComponent.h"
#include "Weapon/Weapon.h"
#include "Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HUD/BlasterHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Sound/SoundCue.h"
#include "Weapon/Projectile.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 350.0f;
	bCanFire = true;
}

void UCombatComponent::InitializeCarriedAmmo()
{
	for (const FCarriedAmmoConfig& CarriedAmmoData : CarriedAmmoConfigs)
	{
		CarriedAmmoMap.Emplace(CarriedAmmoData.WeaponType, CarriedAmmoData.InitialCarriedAmmo);
	}
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;
	case ECombatState::ECS_ThrowGrenade:
		if (Character && !Character->IsLocallyControlled())
		{
			HandleThrowGrenade();
			AttachActorToLeftHand(EquippedWeapon);
			ShowAttachedGrenade(true);
		}
		break;
	case ECombatState::ECS_Unoccupied:
		ResetShotgunReloadTracking();
		if (bFireButtonPressed)
		{
			Fire();
		}
		break;
	default:
		break;
	}
}

void UCombatComponent::OnRep_ShotgunEndShellCount()
{
	TryJumpToShotgunEnd();
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Character && Character->GetFollowCamera())
	{
		DefaultFOV = Character->GetFollowCamera()->FieldOfView;
		CurrentFOV = DefaultFOV;
	}
	
	InitializeCarriedAmmo();
	
}

void UCombatComponent::PlayEquipWeaponSound(AWeapon* WeaponToPlay)
{
	if (Character && WeaponToPlay && WeaponToPlay->GetEquipSound())
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			WeaponToPlay->GetEquipSound(),
			Character->GetActorLocation()
		);
	}
}

FString UCombatComponent::GetWeaponTypeDisplayName(EWeaponType WeaponType)
{
	const UEnum* EnumPtr = StaticEnum<EWeaponType>();
	if (!EnumPtr || WeaponType == EWeaponType::EWT_MAX) return FString("");

	return EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(WeaponType)).ToString();
}

void UCombatComponent::DropWeaponIfEquiped()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->OnAmmoChanged.RemoveDynamic(this, &UCombatComponent::OnWeaponAmmoChanged);
		EquippedWeapon->DroppedWeapon();
	}
}

void UCombatComponent::SetCarriedAmmoFromCarriedAmmoMap()
{
	if (!EquippedWeapon)
		return;

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
}

void UCombatComponent::ReloadEmptyWeapon()
{
	if (EquippedWeapon && EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr)	return;
	//Drop Current Weapon If Equipped, and UnBind The Delegate
	DropWeaponIfEquiped();
	EquippedWeapon = WeaponToEquip;
	//add delegate
	EquippedWeapon->OnAmmoChanged.AddDynamic(this, &UCombatComponent::OnWeaponAmmoChanged);

	// Set CarriedAmmo From CarriedAmmo Map
	SetCarriedAmmoFromCarriedAmmoMap();
	
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	//Attach Weapon To Right Hand
	AttachActorToRightHand(EquippedWeapon);
	//BroadCast CurrentAmmo To This Class, Then Call 
	EquippedWeapon->BroadcastCurrentAmmo();
	//set Owner
	EquippedWeapon->SetOwner(Character);
	//Update Weapon Type and CarriedAmmo In HUD
	UpdateCurrentWeaponTypeInHUD();
	UpdateCurrentCarriedAmmoInHUD();
	
	//Play Equipped Sound
	PlayEquipWeaponSound(EquippedWeapon);
	
	// Reload ammo when weapon ammo is empty
	ReloadEmptyWeapon();
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr)
	{
		return;
	}

	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(SecondaryWeapon);
	PlayEquipWeaponSound(SecondaryWeapon);

	if (Character)
	{
		SecondaryWeapon->SetOwner(Character);
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!Character || !WeaponToEquip)	return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	if (EquippedWeapon != nullptr && SecondaryWeapon == nullptr)
	{
		EquipSecondaryWeapon(WeaponToEquip);
	}
	else
	{
		EquipPrimaryWeapon(WeaponToEquip);
	}
	
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::DroppedWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->OnAmmoChanged.RemoveDynamic(this, &UCombatComponent::OnWeaponAmmoChanged);
		EquippedWeapon->DroppedWeapon();
	}
}

void UCombatComponent::DropOrDestroyWeapon(AWeapon* Weapon)
{
	if (Weapon == nullptr)
	{
		return;
	}
	
	if (Weapon->IsDestroyWeapon())
	{
		Weapon->OnAmmoChanged.RemoveDynamic(this, &UCombatComponent::OnWeaponAmmoChanged);
		Weapon->Destroy();
	}
	else
	{
		Weapon->DroppedWeapon();
	}
}

void UCombatComponent::DropOrDestroyWeapons()
{
	DropOrDestroyWeapon(EquippedWeapon);
	DropOrDestroyWeapon(SecondaryWeapon);
}

void UCombatComponent::SetAiming(bool bAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr)
		return;
	bIsAiming = bAiming;
	ServerSetAiming(bAiming);
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}

	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Character->ShowSniperScopeWidget(bIsAiming);
	}
}

ABlasterPlayerController* UCombatComponent::GetBlasterPlayerController()
{
	if (!Character)
		return nullptr;
	
	PlayerController = PlayerController == nullptr ? Cast<ABlasterPlayerController>(Character->GetController()) : PlayerController;
	return PlayerController;
}

void UCombatComponent::UpdateCurrentWeaponTypeInHUD()
{
	PlayerController = GetBlasterPlayerController();

	if (PlayerController && Character->IsLocallyControlled())
	{
		PlayerController->UpdateHUDWeaponType(GetWeaponTypeDisplayName(EquippedWeapon->GetWeaponType()));
	}
}

void UCombatComponent::UpdateCurrentCarriedAmmoInHUD()
{
	PlayerController = GetBlasterPlayerController();

	if (PlayerController && Character->IsLocallyControlled())
	{
		PlayerController->UpdateHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::OnRep_EquippedWeapon(AWeapon* LastWeapon)
{
	if (LastWeapon)
	{
		LastWeapon->OnAmmoChanged.RemoveDynamic(this, &UCombatComponent::OnWeaponAmmoChanged);
	}
	
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->OnAmmoChanged.AddDynamic(this, &UCombatComponent::OnWeaponAmmoChanged);
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);
		EquippedWeapon->BroadcastCurrentAmmo();
		
		PlayEquipWeaponSound(EquippedWeapon);
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;

		UpdateCurrentWeaponTypeInHUD();
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (SecondaryWeapon && Character)
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
		AttachActorToBackpack(SecondaryWeapon);
		PlayEquipWeaponSound(SecondaryWeapon);
		SecondaryWeapon->SetOwner(Character);
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	if (EquippedWeapon == nullptr) return;
	
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::ShotgunShellReload()
{
	++LocalShotgunShellCount;
	TryJumpToShotgunEnd();
	
	if (Character && Character->HasAuthority())
	{
		UpdateShotgunAmmoValues();
	}
}

bool UCombatComponent::CanPickupAmmo(EWeaponType WeaponType) const
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		int32 CurrentMaxCarriedAmmo = GetWeaponMaxCarriedAmmo(WeaponType);
		
		//When CurrentCarried Ammo less than the MaxCarriedAmmo that the Current Weapon has, return true
		if (CarriedAmmoMap[WeaponType] < CurrentMaxCarriedAmmo)
			return true;
	}
	return false;
}

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		int32 CurrentMaxCarriedAmmo = GetWeaponMaxCarriedAmmo(WeaponType);
		int32 AmmoSpace = FMath::Clamp(CurrentMaxCarriedAmmo - CarriedAmmoMap[WeaponType], 0, CurrentMaxCarriedAmmo);

		int32 CarriedAmmoToReload = AmmoAmount < AmmoSpace ? AmmoAmount : AmmoSpace;
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + CarriedAmmoToReload, 0, CurrentMaxCarriedAmmo);
	}

	//Server Controlled Character Update CarriedAmmo In HUD
	if (EquippedWeapon && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		CarriedAmmo = CarriedAmmoMap[WeaponType];
		UpdateCarriedAmmoUI();
		//Reload when the weapon has no ammo
		if (EquippedWeapon && CarriedAmmo > 0 && EquippedWeapon->IsEmpty())
		{
			Reload();
		}
	}
}

void UCombatComponent::SwapWeapons_Implementation()
{
	if (!Character || !CanSwapWeapon())
	{
		return;
	}

	if (CombatState != ECombatState::ECS_Unoccupied)
	{
		return;
	}

	EquippedWeapon->OnAmmoChanged.RemoveDynamic(this, &UCombatComponent::OnWeaponAmmoChanged);
	Swap(EquippedWeapon, SecondaryWeapon);

	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->OnAmmoChanged.AddDynamic(this, &UCombatComponent::OnWeaponAmmoChanged);
	EquippedWeapon->BroadcastCurrentAmmo();
	EquippedWeapon->SetOwner(Character);

	SecondaryWeapon->OnAmmoChanged.RemoveDynamic(this, &UCombatComponent::OnWeaponAmmoChanged);
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(SecondaryWeapon);
	SecondaryWeapon->SetOwner(Character);

	SetCarriedAmmoFromCarriedAmmoMap();
	UpdateCurrentCarriedAmmoInHUD();
	UpdateCurrentWeaponTypeInHUD();
	PlayEquipWeaponSound(EquippedWeapon);
}

void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr)	return;
	
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		EquippedWeapon->AddAmmo(1);
		bCanFire = true;
		UpdateCarriedAmmoUI();
	}

	//If Shotgun's Ammo is Full, Or Carried Ammo is 0, Jump To End Section
	if (CarriedAmmo <= 0 || EquippedWeapon->IsFull())
	{
		ShotgunEndShellCount = LocalShotgunShellCount;
		bShotgunEndConsumed = true;
		JumpToShotgunEnd();
	}
}

void UCombatComponent::JumpToShotgunEnd()
{
	if (!Character || !Character->GetMesh())
		return;

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	
	if (AnimInstance && Character->GetReloadMontage())
	{
		AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	AttachActorToSocket(ActorToAttach, FName("RightHandSocket"));
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (!EquippedWeapon)
		return;
	bool bUsePistolSocket = EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol ||
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SubmachineGun;
	FName SocketName = bUsePistolSocket ? FName("PistolSocket") : FName("LeftHandSocket");
	
	AttachActorToSocket(ActorToAttach, SocketName);
}

void UCombatComponent::AttachActorToBackpack(AActor* ActorToAttach)
{
	AttachActorToSocket(ActorToAttach, FName("BackpackSocket"));
}

void UCombatComponent::AttachActorToSocket(AActor* ActorToAttach, FName SocketName)
{
	if (!Character || !Character->GetMesh() || !ActorToAttach)
		return;
	
	if (const USkeletalMeshSocket* Socket = Character->GetMesh()->GetSocketByName(SocketName))
	{
		Socket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::ShowAttachedGrenade(bool bShowGrenade)
{
	if (Character && Character->GetAttachedGrenade())
	{
		Character->GetAttachedGrenade()->SetVisibility(bShowGrenade);
	}
}

void UCombatComponent::OnRep_Grenades()
{
	ABlasterPlayerController* Controller = GetBlasterPlayerController();
	if (Controller)
	{
		Controller->UpdateGrenadeAmount(Grenades);
	}
}

int32 UCombatComponent::GetWeaponMaxCarriedAmmo(EWeaponType WeaponType) const
{
	for (const FCarriedAmmoConfig& CarriedAmmoData : CarriedAmmoConfigs)
	{
		if (CarriedAmmoData.WeaponType == WeaponType)
			return CarriedAmmoData.MaxCarriedAmmo;
	}
	//If Character don't have this weapon, return 0
	return 0;
}

bool UCombatComponent::CanSwapWeapon() const
{
	return EquippedWeapon && SecondaryWeapon;
}

FString UCombatComponent::GetWeaponTypeText()
{
	if (!EquippedWeapon)
	{
		return "";
	}

	return GetWeaponTypeDisplayName(EquippedWeapon->GetWeaponType());
}

void UCombatComponent::ServerSpawnGrenade_Implementation(const FVector_NetQuantize& HitLocation)
{
	if (!Character || !GrenadeClass || !Character->GetAttachedGrenade())
	{
		return;
	}

	if (CombatState != ECombatState::ECS_ThrowGrenade || Grenades <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector StartingLocation = Character->GetAttachedGrenade()->GetComponentLocation();
	FVector ToTarget = HitLocation - StartingLocation;
	const FVector LaunchDirection = ToTarget.IsNearlyZero() ? Character->GetActorForwardVector() : ToTarget.GetSafeNormal();
	const FVector SpawnLocation = StartingLocation + LaunchDirection * 100.f;
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;

	if (AProjectile* SpawnedGrenade = World->SpawnActor<AProjectile>(
		GrenadeClass,
		SpawnLocation,
		ToTarget.Rotation(),
		SpawnParams))
	{
		Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);

		if (Character->IsLocallyControlled())
		{
			if (ABlasterPlayerController* Controller = GetBlasterPlayerController())
			{
				Controller->UpdateGrenadeAmount(Grenades);
			}
		}
	}
}

void UCombatComponent::ResetShotgunReloadTracking()
{
	LocalShotgunShellCount = 0;
	bShotgunEndConsumed = false;
}

void UCombatComponent::TryJumpToShotgunEnd()
{
	if (bShotgunEndConsumed)
	{
		return;
	}

	if (ShotgunEndShellCount == 0)
	{
		return;
	}

	if (LocalShotgunShellCount < ShotgunEndShellCount)
	{
		return;
	}

	bShotgunEndConsumed = true;
	UE_LOG(LogTemp, Warning, TEXT("Shotgun end consumed at shell %d"), LocalShotgunShellCount);
	JumpToShotgunEnd();
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	NetMulticastFire(TraceHitTarget);
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr || Character == nullptr)
	{
		return;
	}

	if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun &&
		CombatState == ECombatState::ECS_Reloading)
	{
		Character->PlayFireMontage(bIsAiming);
		EquippedWeapon->Fire(TraceHitTarget);
		CombatState = ECombatState::ECS_Unoccupied;
		return;
	}

	if (CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bIsAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewPortSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewPortSize);
	}

	FVector2D CrosshairLocation(ViewPortSize.X / 2, ViewPortSize.Y / 2);
	FVector WorldLocation;
	FVector WorldDirection;
	
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		WorldLocation,
		WorldDirection
	);

	if (bScreenToWorld)
	{
		FVector Start = WorldLocation;
		//Set the start position in front of the character, avoiding collisions with any characters behind it.
		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += WorldDirection * (DistanceToCharacter + 100.0f);
		}
		FVector End = WorldLocation + WorldDirection * LINETRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECC_Visibility
			);

		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUDPackage.CrossHairColor = FColor::Red;
		}
		else
		{
			HUDPackage.CrossHairColor = FColor::White;
		}
		
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	//check character is valid
	if (Character == nullptr || Character->GetController() == nullptr) return;

	PlayerController = PlayerController == nullptr ? Cast<ABlasterPlayerController>(Character->GetController()) : PlayerController;
	if (PlayerController)
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(PlayerController->GetHUD()) : HUD;
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrossHairCenter = EquippedWeapon->CrossHairCenter;
				HUDPackage.CrossHairLeft = EquippedWeapon->CrossHairLeft;
				HUDPackage.CrossHairRight = EquippedWeapon->CrossHairRight;
				HUDPackage.CrossHairTop = EquippedWeapon->CrossHairTop;
				HUDPackage.CrossHairBottom = EquippedWeapon->CrossHairBottom;
			}
			else
			{
				HUDPackage.CrossHairCenter = nullptr;
				HUDPackage.CrossHairLeft = nullptr;
				HUDPackage.CrossHairRight = nullptr;
				HUDPackage.CrossHairTop = nullptr;
				HUDPackage.CrossHairBottom = nullptr;
			}

			//Calculate CrossHairSpread
			// we need map speed form [0, MaxWalkSpeed] ~ [0, 1]
			FVector2D WalkSpeedRange(0, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange,VelocityMultiplierRange,Velocity.Size());
			//Spread CrossHair when jump, we use deltaTime to interpolate CrosshairInAirFactor 
			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}
			//if aiming, then the crosshairAimFactor will interpolate
			if (bIsAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.0f);
				CrosshairTraceFactor = 0.f;
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.0f, DeltaTime, 30.f);
				if (HUDPackage.CrossHairColor == FColor::Red)
				{
					CrosshairTraceFactor = FMath::FInterpTo(CrosshairTraceFactor, 0.5f, DeltaTime, 30.f);
				}
				else
				{
					CrosshairTraceFactor = FMath::FInterpTo(CrosshairTraceFactor, 0.0f, DeltaTime, 30.f);	
				}
			}
			
			//Shooting Factor will Interpolate to 0
			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);

			HUDPackage.CrossHairSpread = 0.5 +
							CrosshairVelocityFactor +
							CrosshairInAirFactor -
							CrosshairAimFactor -
								CrosshairTraceFactor +
							CrosshairShootingFactor;
			HUD->SetHUDPackage(HUDPackage);
		}
	} 
}

void UCombatComponent::Fire()
{
	if (CanFire())
	{
		LocalFire(HitTarget);
		ServerFire(HitTarget);
		if (EquippedWeapon)
		{
			CrosshairInAirFactor = 0.75f;
		}
		bCanFire = false;
		StartFireTimer();
	}
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr) return;
	
	Character->GetWorldTimerManager().SetTimer(FireTimerHandle,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->GetFireDelay());
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;
	
	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->GetAutomatic())
	{
		Fire();
	}

	ReloadEmptyWeapon();
}

void UCombatComponent::OnWeaponAmmoChanged(int32 NewAmmo)
{
	if (!Character)
		return;
	
	PlayerController = PlayerController == nullptr ? Cast<ABlasterPlayerController>(Character->GetController()) : PlayerController;
	if (PlayerController)
	{
		PlayerController->UpdateHUDWeaponAmmo(NewAmmo);
	}
}

void UCombatComponent::HandleReload()
{
	ShotgunEndShellCount = 0;
	ResetShotgunReloadTracking();
	if (Character)
	{
		Character->PlayReloadMontage();
	}
}

void UCombatComponent::HandleThrowGrenade()
{
	if (Character)
	{
		Character->PlayThrowGrenadeMontage();
	}
}

void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr)	return;
	int32 AmmoToReload = AmountToReload();
	if (AmmoToReload <= 0)
		return;

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= AmmoToReload;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		UpdateCarriedAmmoUI();
	}
	EquippedWeapon->AddAmmo(AmmoToReload);
}

void UCombatComponent::UpdateCarriedAmmoUI()
{
	PlayerController = PlayerController == nullptr ? Cast<ABlasterPlayerController>(Character->GetController()) : PlayerController;
	if (PlayerController)
	{
		PlayerController->UpdateHUDCarriedAmmo(CarriedAmmo);
	}
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetWeaponAmmo();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried  = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(AmountCarried, RoomInMag);
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return 0;
}

void UCombatComponent::ServerReload_Implementation()
{
	if (!Character || !EquippedWeapon)	return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	if (!EquippedWeapon->IsNeedReload() || CarriedAmmo <= 0) return;
	CombatState = ECombatState::ECS_Reloading;
	
	HandleReload();
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (!CanThrowGrenade())
		return;
	
	CombatState = ECombatState::ECS_ThrowGrenade;
	HandleThrowGrenade();
	AttachActorToLeftHand(EquippedWeapon);
	ShowAttachedGrenade(true);
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr)	return;

	if (bIsAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

bool UCombatComponent::CanFire() const
{
	if (!EquippedWeapon)	return false;
	
	if (!EquippedWeapon->IsEmpty() &&
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun &&
		CombatState == ECombatState::ECS_Reloading &&
		bCanFire
		)
		return true;
	
	return EquippedWeapon->GetWeaponAmmo() > 0 &&
		    bCanFire &&
		   	CombatState == ECombatState::ECS_Unoccupied;
}

bool UCombatComponent::CanThrowGrenade() const
{
	return Character && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon && Grenades > 0;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	UpdateCurrentCarriedAmmoInHUD();
}

void UCombatComponent::NetMulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character && Character->IsLocallyControlled())
	{
		return;
	}

	LocalFire(TraceHitTarget);
}

void UCombatComponent::ServerSetAiming_Implementation(bool bAiming)
{
	bIsAiming = bAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled())
	{
		//Calculate Under Crosshairs Trace Target
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;
		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bIsAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, ShotgunEndShellCount);
	DOREPLIFETIME_CONDITION(UCombatComponent, Grenades, COND_OwnerOnly);
}

void UCombatComponent::Reload()
{
	if (!EquippedWeapon || !EquippedWeapon->IsNeedReload())	return;

	if (CombatState != ECombatState::ECS_Unoccupied)
	{
		return;
	}

	if (CarriedAmmo > 0)
	{
		ServerReload();
	}
}

void UCombatComponent::ThrowGrenade()
{
	if (!CanThrowGrenade())
	{
		return;
	}

	//Server and Local Controlled Client Call
	CombatState = ECombatState::ECS_ThrowGrenade;
	HandleThrowGrenade();
	AttachActorToLeftHand(EquippedWeapon);
	ShowAttachedGrenade(true);
	
	if (!Character->HasAuthority())
	{
		ServerThrowGrenade();
	}
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;

	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}

	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::FinishThrowGrenade()
{
	if (Character && Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
	}
	AttachActorToRightHand(EquippedWeapon);
}

void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);

	if (Character && Character->IsLocallyControlled())
	{
		ServerSpawnGrenade(HitTarget);
	}
}
