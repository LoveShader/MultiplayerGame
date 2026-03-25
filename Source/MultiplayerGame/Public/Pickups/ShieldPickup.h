// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "ShieldPickup.generated.h"

UCLASS()
class MULTIPLAYERGAME_API AShieldPickup : public APickup
{
	GENERATED_BODY()

public:
	AShieldPickup();

protected:
	virtual void BeginPlay() override;
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

private:
	UPROPERTY(EditAnywhere)
	float ShieldReplenishAmount = 100.0f;

	UPROPERTY(EditAnywhere)
	float ShieldReplenishTime = 5.0f;
};
