// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "SpeedPickup.generated.h"

UCLASS()
class MULTIPLAYERGAME_API ASpeedPickup : public APickup
{
	GENERATED_BODY()

public:
	ASpeedPickup();

protected:
	virtual void BeginPlay() override;
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

private:
	UPROPERTY(EditAnywhere, Category = "Speed Buff")
	float BaseSpeedBuff = 1600.0f;

	UPROPERTY(EditAnywhere, Category = "Speed Buff")
	float CrouchSpeedBuff = 850.0f;

	UPROPERTY(EditAnywhere, Category = "Speed Buff")
	float SpeedBuffTime = 10.0f;
};
