// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/JumpPickup.h"

#include "BlasterComponents/BuffComponent.h"
#include "Character/BlasterCharacter.h"

AJumpPickup::AJumpPickup()
{
}

void AJumpPickup::BeginPlay()
{
	Super::BeginPlay();
}

void AJumpPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		if (UBuffComponent* Buff = BlasterCharacter->GetBuff())
		{
			Buff->BuffJump(JumpZVelocityBuff, JumpBuffTime);
			Destroy();
		}
	}
}
