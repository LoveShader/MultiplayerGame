// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterAnimInstance.h"

#include "Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Widgets/Text/ISlateEditableTextWidget.h"

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

	bIsCrouched = BlasterCharacter->bIsCrouched;

	bIsAiming = BlasterCharacter->IsAiming();

	//Character's base aiming rotation, generally aligned with the controller's rotation.
	//if (!BlasterCharacter->HasAuthority() && !BlasterCharacter->IsLocallyControlled())
	//{
	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
	//UE_LOG(LogTemp, Warning, TEXT("AimRotation Yaw: %f"), AimRotation.Yaw);
	//UE_LOG(LogTemp, Warning, TEXT("MovementRotation Yaw: %f"), MovementRotation.Yaw);
	//}
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
}
