// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/AmmoPickup.h"

#include "Interface/AmmoReceiverInterface.h"

AAmmoPickup::AAmmoPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	
}

void AAmmoPickup::BeginPlay()
{
	Super::BeginPlay();
	
}

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (OtherActor && OtherActor->GetClass()->ImplementsInterface(UAmmoReceiverInterface::StaticClass()))
	{
		IAmmoReceiverInterface* AmmoReceiver = Cast<IAmmoReceiverInterface>(OtherActor);
		//If Other Actor is nullptr, and it can pickup Ammo, Then Call ReceiveAmmo to pickup.Finally Call Destroy
		// To Play Pickup Sound
		if (AmmoReceiver && AmmoReceiver->CanReceiveAmmo(WeaponType))
		{
			AmmoReceiver->ReceiveAmmo(WeaponType, AmmoAmount);
			Destroy();
		}
	}
}

void AAmmoPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

