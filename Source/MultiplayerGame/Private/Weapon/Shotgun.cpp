// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Shotgun.h"

#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"

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
		FVector Start = SocketTransform.GetLocation();
		TMap<ABlasterCharacter*, uint32> HitMap;
		for (const FVector_NetQuantize& HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

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
		for (const auto& HitPair : HitMap)
		{
			if (HitPair.Key && HasAuthority() && InstigatorController)
			{
				{
					UGameplayStatics::ApplyDamage(
						HitPair.Key,
						static_cast<float>(HitPair.Value) * Damage,
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
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
