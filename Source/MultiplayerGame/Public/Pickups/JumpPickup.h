// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"
#include "JumpPickup.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERGAME_API AJumpPickup : public APickup
{
	GENERATED_BODY()

public:
	AJumpPickup();

protected:
	virtual void BeginPlay() override;
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump Buff", meta = (AllowPrivateAccess = "true"))
	float JumpZVelocityBuff = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump Buff", meta = (AllowPrivateAccess = "true"))
	float JumpBuffTime = 30.0f;
};
