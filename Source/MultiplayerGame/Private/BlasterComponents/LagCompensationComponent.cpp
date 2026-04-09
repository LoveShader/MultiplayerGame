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
}


void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SaveFrameHistory();

	if (FrameHistory.GetHead())
	{
		ShowFramePackage(FrameHistory.GetHead()->GetValue(), FColor::Red);
	}
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
			4.f
		);
	}
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(
	ABlasterCharacter* HitCharacter,
	float HitTime,
	const FVector_NetQuantize& HitLocation,
	const FVector_NetQuantize& HitTarget
)
{
	FFramePackage FrameToCheck;
	bool bShouldInterpolate = true;

	if (
		HitCharacter == nullptr ||
		HitCharacter->GetLagCompensation() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetHead() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetTail() == nullptr
	)
	{
		return FServerSideRewindResult{false, false};
	}

	const ULagCompensationComponent* HitCharacterLagCompensation = HitCharacter->GetLagCompensation();
	const float NewestHistoryTime = HitCharacterLagCompensation->FrameHistory.GetHead()->GetValue().Time;
	const float OldestHistoryTime = HitCharacterLagCompensation->FrameHistory.GetTail()->GetValue().Time;

	if (HitTime < OldestHistoryTime)
	{
		return FServerSideRewindResult{false, false};
	}

	if (HitTime >= NewestHistoryTime)
	{
		FrameToCheck = HitCharacterLagCompensation->FrameHistory.GetHead()->GetValue();
		bShouldInterpolate = false;
	}
	else if (FMath::IsNearlyEqual(HitTime, OldestHistoryTime))
	{
		FrameToCheck = HitCharacterLagCompensation->FrameHistory.GetTail()->GetValue();
		bShouldInterpolate = false;
	}
	else
	{
		TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = HitCharacterLagCompensation->FrameHistory.GetHead();
		TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = HitCharacterLagCompensation->FrameHistory.GetHead();

		while (Older != nullptr && Older->GetValue().Time > HitTime)
		{
			if (Older->GetNextNode() == nullptr)
			{
				break;
			}

			Older = Older->GetNextNode();
			if (Older->GetValue().Time > HitTime)
			{
				Younger = Older;
			}
		}

		if (Older != nullptr && FMath::IsNearlyEqual(Older->GetValue().Time, HitTime))
		{
			FrameToCheck = Older->GetValue();
			bShouldInterpolate = false;
		}

		if (bShouldInterpolate)
		{
			FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
		}
	}
	return ConfirmHit(FrameToCheck, HitCharacter, HitLocation, HitTarget);
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package,
	ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitTarget)
{
	if (HitCharacter == nullptr)	return FServerSideRewindResult{false, false};

	//Cached Current FramePackage
	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);

	//Then Move Box Components to The FrameToCheck's reported Location
	MoveBoxes(HitCharacter, Package);
	//Disable Mesh Collision
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// Enable collision for the head first
	UBoxComponent* HeadBox =  *HitCharacter->GetHitCollisionBoxes().Find(FName("head"));
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HeadBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	
	//Get TraceEnd
	FHitResult HitResult;
	FVector TraceEnd = TraceStart + (HitTarget - TraceStart) * 1.25;
	UWorld* World = GetWorld();
	if (World)
	{
		World->LineTraceSingleByChannel(HitResult,
			TraceStart,
			TraceEnd,
			ECollisionChannel::ECC_Visibility);

		if (HitResult.bBlockingHit)	//we hit the head, return early
		{
			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryOnly);
			return FServerSideRewindResult{ true, true };
		}
		else //didn't hit the head, check the rest of the body
		{
			for (auto& HitBoxPair : HitCharacter->GetHitCollisionBoxes())
			{
				if (HitBoxPair.Value == nullptr)
					continue;
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
			}
			
			World->LineTraceSingleByChannel(HitResult,
				TraceStart,
				TraceEnd,
				ECollisionChannel::ECC_Visibility);

			if (HitResult.bBlockingHit)
			{
				ResetHitBoxes(HitCharacter, CurrentFrame);
				EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryOnly);
				return FServerSideRewindResult{ true, false };
			}
		}
	}

	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryOnly);
	return FServerSideRewindResult{ false, false };
}

void ULagCompensationComponent::CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (HitCharacter == nullptr)	return;

	//For Loop Get Every Box's info, and Cached it to the FramePackage,Then it will Restore after server trace
	for (const TPair<FName, UBoxComponent*>& HitBoxPair : HitCharacter->GetHitCollisionBoxes())
	{
		if (HitBoxPair.Value == nullptr)
			continue;
		FBoxInformation BoxInfo;
		BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
		BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
		BoxInfo.BoxExtent = HitBoxPair.Value->GetScaledBoxExtent();
		//put the {BoxComponent Name, BoxInformation} to the PackageFrame
 		OutFramePackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
	}
}

void ULagCompensationComponent::MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr)	return;

	for (TPair<FName, UBoxComponent*>& HitBoxPair : HitCharacter->GetHitCollisionBoxes())
	{
		if (HitBoxPair.Value == nullptr)
			continue;

		HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
		HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
		HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
	}
}

void ULagCompensationComponent::ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr)	return;

	for (TPair<FName, UBoxComponent*>& HitBoxPair : HitCharacter->GetHitCollisionBoxes())
	{
		if (HitBoxPair.Value == nullptr)
			continue;

		HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
		HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
		HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
		HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter,
	ECollisionEnabled::Type CollisionEnabled)
{
	if (HitCharacter == nullptr || HitCharacter->GetMesh() == nullptr)	return;

	HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(
	const FFramePackage& OlderFrame,
	const FFramePackage& YoungerFrame,
	float HitTime
) const
{
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	const float InterpFraction = Distance > KINDA_SMALL_NUMBER
		? FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.f, 1.f)
		: 0.f;

	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;

	for (const TPair<FName, FBoxInformation>& YoungerPair : YoungerFrame.HitBoxInfo)
	{
		const FName& BoxName = YoungerPair.Key;
		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxName];
		const FBoxInformation& YoungerBox = YoungerPair.Value;

		FBoxInformation InterpBoxInfo;
		InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction);
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction);
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent;

		InterpFramePackage.HitBoxInfo.Add(BoxName, InterpBoxInfo);
	}

	return InterpFramePackage;
}

void ULagCompensationComponent::SaveFrameHistory()
{
	if (Character == nullptr)
	{
		Character = Cast<ABlasterCharacter>(GetOwner());
	}

	if (Character == nullptr)
	{
		return;
	}

	FFramePackage ThisFrame;
	SaveFramePackage(ThisFrame);

	if (FrameHistory.Num() <= 1)
	{
		FrameHistory.AddHead(ThisFrame);
		return;
	}

	float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
	while (HistoryLength > MaxRecordTime && FrameHistory.GetTail() != nullptr)
	{
		FrameHistory.RemoveNode(FrameHistory.GetTail());
		if (FrameHistory.GetHead() == nullptr || FrameHistory.GetTail() == nullptr)
		{
			break;
		}
		HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
	}

	FrameHistory.AddHead(ThisFrame);
}
