// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/LagCompensationComponent.h"
#include "Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"


ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();

	FFramePackage Package;
	SaveFramePackage(Package);
	ShowFramePackage(Package, FColor::Red);
}


void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	if (Character == nullptr)
	{
		Character = Cast<ABlasterCharacter>(GetOwner());
	}

	if (Character == nullptr)
	{
		return;
	}

	Package.Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	Package.HitBoxInfo.Empty();

	for (const TPair<FName, UBoxComponent*>& BoxPair : Character->GetHitCollisionBoxes())
	{
		if (BoxPair.Value == nullptr)
		{
			continue;
		}

		FBoxInformation BoxInformation;
		BoxInformation.Location = BoxPair.Value->GetComponentLocation();
		BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
		BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();

		Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
	}
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color) const
{
	if (GetWorld() == nullptr)
	{
		return;
	}

	for (const TPair<FName, FBoxInformation>& HitBoxPair : Package.HitBoxInfo)
	{
		DrawDebugBox(
			GetWorld(),
			HitBoxPair.Value.Location,
			HitBoxPair.Value.BoxExtent,
			FQuat(HitBoxPair.Value.Rotation),
			Color,
			false,
			10.f
		);
	}
}

