// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

UCLASS()
class MULTIPLAYERGAME_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	
	virtual void Fire(const FVector& HitTarget) override;
protected:
	FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget);
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& FireHit);
	void PlayVFXWhenHitActor(const FHitResult& FireResult) const;
	void PlayVFXWhenFire(const FTransform& SocketTransform) const;
	void PlayBeamEffect(const FVector& Start, const FVector& End) const;

	UPROPERTY(EditAnywhere)
	float Damage = 20.0f;
private:
	UPROPERTY(EditAnywhere)
	class UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticles;

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere)
	class USoundCue* FireSound;

	UPROPERTY(EditAnywhere)
	USoundCue* HitSound;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;

	UPROPERTY(EditAnywhere)
	float HitVolumeMultiplier = 1.0f;
	
	UPROPERTY(EditAnywhere, Category="RandomPitch")
	float HitPitchMultiplierMin = 1.0f;

	UPROPERTY(EditAnywhere, Category="RandomPitch")
	float HitPitchMultiplierMax = 1.0f;
};
