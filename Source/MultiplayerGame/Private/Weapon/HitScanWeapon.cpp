// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitScanWeapon.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
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
			if (BlasterCharacter && HasAuthority() && InstigatorController)
			{
				UGameplayStatics::ApplyDamage(
					BlasterCharacter,
					Damage,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
			PlayVFXWhenHitActor(FireResult);
		}
	}
}

FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.0f, SphereRadius);
	FVector EndLoc = SphereCenter + RandVec;
	FVector ToEndLoc = EndLoc - TraceStart;

	/*
	//Draw Sphere 
	DrawDebugSphere(
		GetWorld(),
		SphereCenter,
		SphereRadius,
		12,
		FColor::Red,
		true
	);
	
	//Draw the point that generate in the sphere
	DrawDebugSphere(
		GetWorld(),
		EndLoc,
		4.0f,
		12,
		FColor::Orange,
		true
	);

	//Draw Line that indicate the trace Line
	DrawDebugLine(
		GetWorld(),
		TraceStart,
		FVector(TraceStart + ToEndLoc * LINETRACE_LENGTH / ToEndLoc.Size()),
		FColor::Cyan,
		true
	);*/

	return FVector(TraceStart + ToEndLoc * LINETRACE_LENGTH / ToEndLoc.Size());
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& FireHit)
{
	UWorld* World = GetWorld();
	FVector TraceEnd = bUseScatter ? TraceEndWithScatter(TraceStart,HitTarget) : TraceStart + (HitTarget - TraceStart) * 1.25f;

	if (World)
	{
		World->LineTraceSingleByChannel(
			FireHit,
			TraceStart,
			TraceEnd,
			ECC_Visibility
		);
	}
	
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
