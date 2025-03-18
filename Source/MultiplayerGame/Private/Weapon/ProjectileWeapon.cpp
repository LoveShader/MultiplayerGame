// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"
#include "Weapon/Projectile.h"
#include "Engine/SkeletalMeshSocket.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	//Only server can Spawn and do hit event
	if (!HasAuthority())	return;
	//generate projectile at muzzleflash socket

	//Get Muzzle flash Socket
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		APawn* ProjectileInstigator = Cast<APawn>(GetOwner());
		if (ProjectileInstigator && ProjectileClass)
		{
			FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
			FVector ToTarget = HitTarget - SocketTransform.GetLocation();
			FRotator TargetRotation = ToTarget.Rotation();
			FActorSpawnParameters SpawnParameters;
			//This projectile's owner is The Weapon owner
			SpawnParameters.Owner = GetOwner();
			SpawnParameters.Instigator = ProjectileInstigator;
			
			UWorld* World = GetWorld();
			if (World)
			{
				//Spawn the Projectile
				World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(),TargetRotation, SpawnParameters);
			}
		}
	}
}
