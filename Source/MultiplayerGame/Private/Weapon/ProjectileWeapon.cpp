// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"
#include "Weapon/Projectile.h"
#include "Engine/SkeletalMeshSocket.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);
	
	//generate projectile at muzzleflash socket
	//Get Muzzle flash Socket
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	UWorld* World = GetWorld();
	
	if (MuzzleFlashSocket && World)
	{
		AProjectile* SpawnedProjectile = nullptr;
		FActorSpawnParameters SpawnParameters;
		//This projectile's owner is The Weapon owner
		SpawnParameters.Owner = GetOwner();
		APawn* ProjectileInstigator = Cast<APawn>(GetOwner());
		SpawnParameters.Instigator = ProjectileInstigator;
		
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();
		if (bUseServerSideRewind)	//use Server side Rewind
		{
			if (ProjectileInstigator->HasAuthority())
			{
				if (ProjectileInstigator->IsLocallyControlled())	//Spawn Projectile(bReplicated = true)
				{	
					SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(),TargetRotation, SpawnParameters);
					SpawnedProjectile->bUseServerSideRewind = false;
					SpawnedProjectile->Damage = Damage;
				}
				else  //Spawn Server side Rewind Projectile, use ssr
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(),TargetRotation, SpawnParameters);
					SpawnedProjectile->bUseServerSideRewind = true;
				}
			}
			else
			{
				if (ProjectileInstigator->IsLocallyControlled())	//Owning Client, spawn ServerSide Rewind Projectile(Not Replicated), and Send RPC to Server
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(),TargetRotation, SpawnParameters);
					SpawnedProjectile->bUseServerSideRewind = true;
					SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
					SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
					SpawnedProjectile->Damage = Damage;
				}
				else
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(),TargetRotation, SpawnParameters);
					SpawnedProjectile->bUseServerSideRewind = false;	
				}
			}
		}
		else
		{
			if (HasAuthority())
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(),TargetRotation, SpawnParameters);
				SpawnedProjectile->bUseServerSideRewind = false;
				SpawnedProjectile->Damage = Damage;
			}
		}
	}
}
