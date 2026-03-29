// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/HitScanWeapon.h"
#include "Shotgun.generated.h"

class ABlasterCharacter;
/**
 * 
 */
UCLASS()
class MULTIPLAYERGAME_API AShotgun : public AHitScanWeapon
{
	GENERATED_BODY()
public:
	virtual void Fire(const FVector& HitTarget) override;
	void FireShotgun(const TArray<FVector_NetQuantize>& HitTargets);
	void ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets);
private:
	UPROPERTY(EditAnywhere)
	uint32 NumberOfPellets = 12;
};
