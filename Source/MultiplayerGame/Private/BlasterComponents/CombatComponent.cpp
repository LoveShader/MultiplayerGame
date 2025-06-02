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

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 350.0f;
	bCanFire = true;
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;
	}
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

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!Character || !WeaponToEquip)	return;

	if (EquippedWeapon)
	{
		EquippedWeapon->OnAmmoChanged.RemoveDynamic(this, &UCombatComponent::OnWeaponAmmoChanged);
		EquippedWeapon->DroppedWeapon();
	}
	EquippedWeapon = WeaponToEquip;
	//add delegate
	EquippedWeapon->OnAmmoChanged.AddDynamic(this, &UCombatComponent::OnWeaponAmmoChanged);
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* RightHandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (!RightHandSocket)	return;
	RightHandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	EquippedWeapon->BroadcastCurrentAmmo();
	//set Owner
	EquippedWeapon->SetOwner(Character);
	PlayerController = PlayerController == nullptr ? Cast<ABlasterPlayerController>(Character->GetController()) : PlayerController;
	if (PlayerController)
	{
		PlayerController->UpdateHUDCarriedAmmo(CarriedAmmo);
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

void UCombatComponent::SetAiming(bool bAiming)
{
	bIsAiming = bAiming;
	ServerSetAiming(bAiming);
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
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
		const USkeletalMeshSocket* RightHandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (!RightHandSocket)	return;
		RightHandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		EquippedWeapon->BroadcastCurrentAmmo();
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
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

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	NetMulticastFire(TraceHitTarget);
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
		0.15f);
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;
	
	bCanFire = true;
	if (bFireButtonPressed)
	{
		Fire();
	}
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
	if (Character)
	{
		Character->PlayReloadMontage();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
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
	return EquippedWeapon->GetWeaponAmmo() > 0 && bCanFire;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	PlayerController = PlayerController == nullptr ? Cast<ABlasterPlayerController>(Character->GetController()) : PlayerController;
	if (PlayerController)
	{
		PlayerController->UpdateHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::NetMulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character)
	{
		Character->PlayFireMontage(bIsAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
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
	DOREPLIFETIME(UCombatComponent, bIsAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
}

void UCombatComponent::Reload()
{
	if (!EquippedWeapon)	return;
	
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading)
	{
		ServerReload();
	}
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;

	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
	}
}
