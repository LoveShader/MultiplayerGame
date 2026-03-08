// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Shotgun.h"

#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"

void AShotgun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget);
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();
		TMap<ABlasterCharacter*, uint32> HitMap;
		for (uint32 i = 0; i < NumberOfPellets; i++)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget,FireHit);

			if (FireHit.bBlockingHit)
			{
				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
				if (BlasterCharacter && HasAuthority())
				{
					if (HitMap.Contains(BlasterCharacter))
					{
						HitMap[BlasterCharacter]++;
					} else
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
						HitPair.Value * Damage,
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
			}
		}
	}
}
