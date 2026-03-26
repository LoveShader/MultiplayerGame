// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

class APickup;

UCLASS()
class MULTIPLAYERGAME_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	APickupSpawnPoint();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyedActor);

	void SpawnPickup();

	UPROPERTY(EditAnywhere, Category = PickupSpawning)
	TArray<TSubclassOf<APickup>> PickupClasses;

	UPROPERTY(EditAnywhere, Category = PickupSpawning)
	float SpawnPickupTimeMin = 10.0f;

	UPROPERTY(EditAnywhere, Category = PickupSpawning)
	float SpawnPickupTimeMax = 20.0f;

	UPROPERTY()
	APickup* SpawnedPickup = nullptr;

	FTimerHandle SpawnPickupTimer;
};
