// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterComponents/CombatComponent.h"
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
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!Character || !WeaponToEquip)	return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* RightHandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (!RightHandSocket)	return;
	RightHandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	//set Owner
	EquippedWeapon->SetOwner(Character);
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
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

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
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
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		ServerFire(HitResult.ImpactPoint);
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
		FVector End = WorldLocation + WorldDirection * LINETRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECC_Visibility
			);

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
			FHUDPackage HUDPackage;
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

			HUDPackage.CrossHairSpread = CrosshairVelocityFactor + CrosshairInAirFactor;
			HUD->SetHUDPackage(HUDPackage);
		}
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
	SetHUDCrosshairs(DeltaTime);

	if (Character && Character->IsLocallyControlled())
	{
		//Calculate Under Crosshairs Trace Target
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bIsAiming);
}