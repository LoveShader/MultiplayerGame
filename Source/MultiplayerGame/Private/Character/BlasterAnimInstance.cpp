// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterAnimInstance.h"

#include "Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/Weapon.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (!IsValid(BlasterCharacter))
	{
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
		if (!IsValid(BlasterCharacter))
			return;
	}

	//Get Pawn Speed
	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0;
	Speed = Velocity.Size();

	//Determine whether it is in the air
	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();

	//Determin whether it's accelerating(add a small value to avoid jitter)
	bAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.0f ? true : false;
	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
	bIsCrouched = BlasterCharacter->bIsCrouched;
	bIsAiming = BlasterCharacter->IsAiming();
	TurningInPlace = BlasterCharacter->GetTurningInPlace();
	bIsElimed = BlasterCharacter->GetIsElimed();

	//Character's base aiming rotation, generally aligned with the controller's rotation.
	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 15.0f);
	YawOffset = DeltaRotation.Yaw;
	
	//Calculate Lean offset 
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float InTerp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.0f);
	Lean = FMath::Clamp(InTerp, -90.0f, 90.0f);

	//get AO_Yaw and AO_Pitch from BlasterCharacter
	AO_Yaw = BlasterCharacter->GetAO_Yaw();
	AO_Pitch = BlasterCharacter->GetAO_Pitch();

	//tweak Weapon Socket Translation
	if (bWeaponEquipped && EquippedWeapon && BlasterCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHand"), RTS_World);

		FVector OutLocation;
		FRotator OutRotation;
		BlasterCharacter->GetMesh()->TransformToBoneSpace("hand_r", LeftHandTransform.GetLocation(),FRotator::ZeroRotator, OutLocation, OutRotation);
		LeftHandTransform.SetLocation(OutLocation);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (BlasterCharacter->IsLocallyControlled())
		{
			FTransform RightHandTransform  = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("Hand_R"), RTS_World);
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.0f);
			bIsLocalControlled = true;
		}

		/*
		FTransform MuzzleSocketTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), RTS_World);
		FVector MuzzleLocation = MuzzleSocketTransform.GetLocation();
		FVector MuzzleRotation = FRotationMatrix(MuzzleSocketTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X);
		
		DrawDebugLine(GetWorld(), MuzzleLocation, MuzzleLocation + MuzzleRotation * 1000, FColor(0,255,0));
		DrawDebugLine(GetWorld(), MuzzleLocation,BlasterCharacter->GetHitTarget(), FColor::Red);*/
	}
}
