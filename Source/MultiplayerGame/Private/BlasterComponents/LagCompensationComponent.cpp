// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/LagCompensationComponent.h"
#include "Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "MultiplayerGame/MultiplayerGame.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Weapon/HitScanWeapon.h"


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

	/*if (FrameHistory.GetHead())
	{
		ShowFramePackage(FrameHistory.GetHead()->GetValue(), FColor::Red);
	}*/
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
	Package.Character = Character;
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

FFramePackage ULagCompensationComponent::GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime)
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
		return FrameToCheck;
	}

	const ULagCompensationComponent* HitCharacterLagCompensation = HitCharacter->GetLagCompensation();
	const float NewestHistoryTime = HitCharacterLagCompensation->FrameHistory.GetHead()->GetValue().Time;
	const float OldestHistoryTime = HitCharacterLagCompensation->FrameHistory.GetTail()->GetValue().Time;

	if (HitTime < OldestHistoryTime)
	{
		return FrameToCheck;
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
	FrameToCheck.Character = HitCharacter;
	return FrameToCheck;
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(
	ABlasterCharacter* HitCharacter,
	float HitTime,
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize& HitTarget
)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitTarget);
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(
	TArray<ABlasterCharacter*> HitCharacters, float HitTime, const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitTargets)
{
	TArray<FFramePackage> FramesToCheck;
	for (ABlasterCharacter* HitCharacter : HitCharacters)
	{
		FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));
	}

	return ShotgunConfirmHit(FramesToCheck, TraceStart, HitTargets);
}

void ULagCompensationComponent::ShotgunServerScoreRequest_Implementation(const TArray<ABlasterCharacter*>& HitCharacters,
	float HitTime, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitTargets)
{
	FShotgunServerSideRewindResult ShotgunHitResult = ShotgunServerSideRewind(HitCharacters, HitTime, TraceStart, HitTargets);

	if (Character == nullptr || Character->Controller == nullptr || Character->GetEquippedWeapon() == nullptr)
		return;
	// For Loop for every Hit Character that in the HitCharacters Map

	// To Be Done:
	// There is a risk, if HitCharacters have duplicate character, then
	// it will receive the damage more than once, so ues map is more safety
	// or remove duplicate elements first
	for (ABlasterCharacter* HitCharacter : HitCharacters)
	{
		if (HitCharacter == nullptr)
			continue;
		
		float HitDamage = 0.0f;
		if (ShotgunHitResult.HeadShots.Contains(HitCharacter))
		{
			float HeadDamage = ShotgunHitResult.HeadShots[HitCharacter] * Character->GetEquippedWeapon()->GetDamage();
			HitDamage += HeadDamage;
		}

		if (ShotgunHitResult.BodyShots.Contains(HitCharacter))
		{
			float BodyDamage = ShotgunHitResult.BodyShots[HitCharacter] * Character->GetEquippedWeapon()->GetDamage();
			HitDamage += BodyDamage;
		}

		if (HitDamage > 0.0f)
		{
			UGameplayStatics::ApplyDamage(
				HitCharacter,
				HitDamage,
				Character->Controller,
				Character->GetEquippedWeapon(),
				UDamageType::StaticClass()
			);
		}
	}
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(
	ABlasterCharacter* HitCharacter,
	float HitTime,
	const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize& HitTarget,
	AHitScanWeapon* DamageCauser
)
{
	if (Character == nullptr || Controller == nullptr || HitCharacter == nullptr || DamageCauser == nullptr)
	{
		return;
	}

	const FServerSideRewindResult ConfirmResult = ServerSideRewind(HitCharacter, HitTime, TraceStart, HitTarget);
	if (ConfirmResult.bHitConfirmed)
	{
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			DamageCauser->GetDamage(),
			Controller,
			DamageCauser,
			UDamageType::StaticClass()
		);
	}
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
	UBoxComponent* const* HeadBoxPtr = HitCharacter->GetHitCollisionBoxes().Find(FName("head"));
	if (HeadBoxPtr == nullptr || *HeadBoxPtr == nullptr)
	{
		ResetHitBoxes(HitCharacter, CurrentFrame);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryOnly);
		return FServerSideRewindResult{ false, false };
	}
	UBoxComponent* HeadBox = *HeadBoxPtr;
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
	
	//Get TraceEnd
	FHitResult HitResult;
	FVector TraceEnd = TraceStart + (HitTarget - TraceStart) * 1.25;
	UWorld* World = GetWorld();
	if (World)
	{
		World->LineTraceSingleByChannel(HitResult,
			TraceStart,
			TraceEnd,
			ECC_HitBox);

		if (HitResult.bBlockingHit)	//we hit the head, return early
		{
			if (HitResult.Component.IsValid())
			{
				UBoxComponent* Box = Cast<UBoxComponent>(HitResult.Component);
				if (Box)
				{
					DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f);
				}
			}
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
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}
			
			World->LineTraceSingleByChannel(HitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox);

			if (HitResult.bBlockingHit)
			{
				if (HitResult.Component.IsValid())
				{
					UBoxComponent* Box = Cast<UBoxComponent>(HitResult.Component);
					if (Box)
					{
						DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Blue, false, 8.f);
					}
				}
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

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(
	const TArray<FFramePackage>& FramesToCheck, const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitTargets)
{
	for (auto& Frame : FramesToCheck)
	{
		if (Frame.Character == nullptr)
			return FShotgunServerSideRewindResult();
	}

	FShotgunServerSideRewindResult ShotgunResult;
	TArray<FFramePackage> CurrentFrames;

	/*
	 * For Loop, doing these things
	 * 1. Cache Current Character's BoxComponent
	 * 2. Move Box Component's To FrameToCheck's Transform
	 * 3. Disable All Character's Mesh Collision
	 */
	for (const FFramePackage& Package : FramesToCheck)
	{
		FFramePackage CurrentFrame;
		CurrentFrame.Character = Package.Character;
		CacheBoxPositions(Package.Character, CurrentFrame);
		MoveBoxes(Package.Character, Package);
		EnableCharacterMeshCollision(Package.Character, ECollisionEnabled::NoCollision);
		CurrentFrames.Add(CurrentFrame);
	}

	/*
	 * First for HeadShot, We need do these things:
	 * 1. Get Head Box Components of All HitCharacters
	 * 2. Do Safety Check, Just think how to do it when check head Box Component failed, there just reset state，and return null hit.
	 * 3. Enable Collision for Head Box Component
	 */
	for (const FFramePackage& Package : FramesToCheck)
	{
		if (Package.HitBoxInfo.Find(FName("head")))
		{
			UBoxComponent* const* HeadBoxPtr = Package.Character->GetHitCollisionBoxes().Find(FName("head"));
			//Safety check,
			if (HeadBoxPtr == nullptr || *HeadBoxPtr == nullptr)
			{
				for (auto CurrentFrame: CurrentFrames)
				{
					ResetHitBoxes(CurrentFrame.Character, CurrentFrame);
					EnableCharacterMeshCollision(CurrentFrame.Character, ECollisionEnabled::QueryOnly);
				}
				return ShotgunResult;
			}
			UBoxComponent* HeadBox = *HeadBoxPtr;
			HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
		}
	}
	
	UWorld* World = GetWorld();
	if (World)
	{
		/*
		 * Then Do LineTrace, for HeadShot, Add the nums of hit to the HeadShots Map
		 * Can Reference to the Shotgun.cpp's Shotgun Fire Function
		 * For each HitTarget, Do Once LineTrace.
		 * If hit the BlasterCharacter, then accumulate the hit times to the HeadShots
		 */
		for (const FVector_NetQuantize& HitTarget : HitTargets)
		{
			FHitResult ConfirmHitResult;
			const FVector TraceEnd = TraceStart + (HitTarget - TraceStart) * 1.25f;
			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
				);
			
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor());
			if (ConfirmHitResult.bBlockingHit && BlasterCharacter)
			{
				if (ConfirmHitResult.Component.IsValid())
				{
					UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
					if (Box)
					{
						DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f);
					}
				}
				if (ShotgunResult.HeadShots.Contains(BlasterCharacter))
				{
					ShotgunResult.HeadShots[BlasterCharacter]++;;
				} else
				{
					ShotgunResult.HeadShots.Emplace(BlasterCharacter, 1);
				}
			}
		}

		/*
		 * After check headshots, then need to check the BodyShots.So need those steps:
		 * 1. Enable All Character's BoxComponents except the headBox
		 * 2. For each HitTarget, Do Once LineTrace.
		 * 3. If hit the BlasterCharacter, then accumulate the hit times to the BodyShots
		 */
		for (const FFramePackage& Package : FramesToCheck)
		{
			for (const auto& HitBoxes : Package.Character->GetHitCollisionBoxes())
			{
				if (HitBoxes.Value == nullptr)
					continue;
				HitBoxes.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				HitBoxes.Value->SetCollisionResponseToChannel(ECC_HitBox,ECollisionResponse::ECR_Block);
			}

			UBoxComponent* HeadBox = Package.Character->GetHitCollisionBoxes()[FName("head")];
			HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Ignore);
		}

		for (const FVector_NetQuantize& HitTarget : HitTargets)
		{
			FHitResult ConfirmHitResult;
			const FVector TraceEnd = TraceStart + (HitTarget - TraceStart) * 1.25f;
			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
				);
			
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor());
			if (BlasterCharacter && ConfirmHitResult.bBlockingHit)
			{
				if (ConfirmHitResult.Component.IsValid())
				{
					UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
					if (Box)
					{
						DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Blue, false, 8.f);
					}
				}
				if (ShotgunResult.BodyShots.Contains(BlasterCharacter))
				{
					ShotgunResult.BodyShots[BlasterCharacter]++;;
				} else
				{
					ShotgunResult.BodyShots.Emplace(BlasterCharacter, 1);
				}
			}
		}
	}

	/*
	 * After LineTrace, We need do these things:
	 * 1. Reset the BoxComponents to CurrentFrame.
	 * 2. Enable Mesh Collision
	 * 3. Disable all the BoxComponents Collision Test
	 */
	for (const FFramePackage& CurrentFrame : CurrentFrames)
	{
		ResetHitBoxes(CurrentFrame.Character, CurrentFrame);
		EnableCharacterMeshCollision(CurrentFrame.Character, ECollisionEnabled::QueryOnly);
	}
	
	return ShotgunResult;
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

	if (!Character->HasAuthority())
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