// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/PickupSpawnPoint.h"

#include "Pickups/Pickup.h"
#include "TimerManager.h"

APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	StartSpawnPickupTimer(nullptr);
}

void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
	if (!HasAuthority())
	{
		return;
	}

	const float SpawnTime = FMath::FRandRange(SpawnPickupTimeMin, SpawnPickupTimeMax);
	GetWorldTimerManager().SetTimer(SpawnPickupTimer, this, &APickupSpawnPoint::SpawnPickup, SpawnTime);
}

void APickupSpawnPoint::SpawnPickup()
{
	if (!HasAuthority() || PickupClasses.Num() == 0)
	{
		return;
	}

	const int32 Selection = FMath::RandRange(0, PickupClasses.Num() - 1);
	SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[Selection], GetActorTransform());

	if (SpawnedPickup)
	{
		SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnPickupTimer);
	}
}
