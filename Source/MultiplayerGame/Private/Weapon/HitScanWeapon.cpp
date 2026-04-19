// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitScanWeapon.h"
#include "BlasterComponents/LagCompensationComponent.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Sound/SoundCue.h"

void AHitScanWeapon::PlayVFXWhenHitActor(const FHitResult& FireResult) const
{
	if (UWorld* World = GetWorld())
	{
		if (ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				World,
				ImpactParticles,
				FireResult.ImpactPoint,
				FireResult.ImpactNormal.Rotation()
			);
		}
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				World,
				HitSound,
				FireResult.ImpactPoint,
				HitVolumeMultiplier,
				FMath::FRandRange(HitPitchMultiplierMin, HitPitchMultiplierMax)
				);
		}
	}
	
}

void AHitScanWeapon::PlayVFXWhenFire(const FTransform& SocketTransform) const
{
	if (UWorld* World = GetWorld())
	{
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				World,
				MuzzleFlash,
				SocketTransform
			);
		}
		if (FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				World,
				FireSound,
				GetActorLocation()
			);
		}
	}
}

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);
 
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
		return;
	
	if (const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash"))
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		FHitResult FireResult;
		WeaponTraceHit(Start, HitTarget,FireResult);
		
		PlayVFXWhenFire(SocketTransform);
		if (FireResult.bBlockingHit)
		{
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireResult.GetActor());
			AController* InstigatorController = OwnerPawn->GetController();
			ABlasterCharacter* OwnerCharacter = Cast<ABlasterCharacter>(OwnerPawn);
			ABlasterPlayerController* BlasterPlayerController = OwnerCharacter ? Cast<ABlasterPlayerController>(InstigatorController) : nullptr;
			if (BlasterCharacter && HasAuthority() && InstigatorController && OwnerPawn->IsLocallyControlled())
			{
				UGameplayStatics::ApplyDamage(
					BlasterCharacter,
					Damage,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
			else if (
				BlasterCharacter &&
				OwnerCharacter &&
				OwnerCharacter->GetLagCompensation() &&
				BlasterPlayerController &&
				OwnerPawn->IsLocallyControlled()
			)
			{
				OwnerCharacter->GetLagCompensation()->ServerScoreRequest(
					BlasterCharacter,
					BlasterPlayerController->GetServerTime() - BlasterPlayerController->GetSingleTripTime(),
					Start,
					HitTarget,
					this
				);
			}
			PlayVFXWhenHitActor(FireResult);
		}
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& FireHit)
{
	UWorld* World = GetWorld();
	FVector TraceEnd = TraceStart + (HitTarget - TraceStart) * 1.25f;

	if (World)
	{
		World->LineTraceSingleByChannel(
			FireHit,
			TraceStart,
			TraceEnd,
			ECC_Visibility
		);
	}

	DrawDebugSphere(GetWorld(),
		FireHit.ImpactPoint,
		16.0f,
		12,
		FColor::Red,
		true);
	
	PlayBeamEffect(TraceStart, FireHit.bBlockingHit ? FireHit.ImpactPoint : TraceEnd);
}

void AHitScanWeapon::PlayBeamEffect(const FVector& Start, const FVector& End) const
{
	if (BeamParticles)
	{
		UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			BeamParticles,
			Start,
			FRotator::ZeroRotator,
			true
		);

		if (Beam)
		{
			Beam->SetVectorParameter(FName("Target"),  End);
		}
	}
}
