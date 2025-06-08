// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileRocket.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERGAME_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()
protected:
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
private:
	UPROPERTY(EditDefaultsOnly)
	float MinDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly)
	float MinInnerRadius = 200.0f;

	UPROPERTY(EditDefaultsOnly)
	float MaxOuterRadius = 500.0f;
};
