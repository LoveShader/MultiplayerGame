// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Shotgun.h"

#include "BlasterComponents/LagCompensationComponent.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerController/BlasterPlayerController.h"

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{
	HitTargets.Reset();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr)
	{
		return;
	}

	const FVector TraceStart = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh()).GetLocation();
	for (uint32 i = 0; i < NumberOfPellets; i++)
	{
		HitTargets.Add(TraceEndWithScatter(TraceStart, HitTarget));
	}
}

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	if (HitTargets.Num() == 0)
	{
		return;
	}

	AWeapon::Fire(HitTargets[0]);
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector TraceStart = SocketTransform.GetLocation();
		TMap<ABlasterCharacter*, uint32> HitMap;
		for (const FVector_NetQuantize& HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(TraceStart, HitTarget, FireHit);

			if (FireHit.bBlockingHit)
			{
				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
				if (BlasterCharacter)
				{
					if (HitMap.Contains(BlasterCharacter))
					{
						HitMap[BlasterCharacter]++;
					}
					else
					{
						HitMap.Emplace(BlasterCharacter, 1);
					}
				}

				PlayVFXWhenHitActor(FireHit);
			}
		}

		AController* InstigatorController = OwnerPawn->GetController();
		TArray<ABlasterCharacter*> HitCharacters;
		for (const auto& HitPair : HitMap)
		{
			if (HitPair.Key && InstigatorController)
			{
				if (HasAuthority() && OwnerPawn->IsLocallyControlled())
				{
					UGameplayStatics::ApplyDamage(
						HitPair.Key,
						static_cast<float>(HitPair.Value) * Damage,
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}

				HitCharacters.Add(HitPair.Key);
			}
		}

		//Client Side Call Server RPC to do Shotgun Server Side Rewind
		ABlasterCharacter* OwnerCharacter = Cast<ABlasterCharacter>(OwnerPawn);
		if (!HasAuthority() && OwnerCharacter && OwnerCharacter->GetLagCompensation() && OwnerCharacter->IsLocallyControlled())
		{
			ABlasterPlayerController* OwnerPlayerController = Cast<ABlasterPlayerController>(OwnerCharacter->GetController());
			if (OwnerPlayerController)
			{
				OwnerCharacter->GetLagCompensation()->ShotgunServerScoreRequest(
					HitCharacters,
					OwnerPlayerController->GetServerTime() - OwnerPlayerController->GetSingleTripTime(),
					TraceStart,
					HitTargets
				);
			}
		}
	}

	
}

void AShotgun::Fire(const FVector& HitTarget)
{
	TArray<FVector_NetQuantize> HitTargets;
	ShotgunTraceEndWithScatter(HitTarget, HitTargets);
	FireShotgun(HitTargets);
}
