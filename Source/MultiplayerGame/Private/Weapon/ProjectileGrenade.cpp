// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileGrenade.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AProjectileGrenade::AProjectileGrenade()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>("GrenadeMesh");
	ProjectileMesh->SetupAttachment(GetRootComponent());
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	//Set Projectile Rotation with it's Velocity
	ProjectileMovement->SetIsReplicated(true);
	ProjectileMovement->bRotationFollowsVelocity = true;
	//Allow Grenade Bounce when it hit something
	ProjectileMovement->bShouldBounce = true;
}

void AProjectileGrenade::Destroyed()
{
	ExplodeDamage();
	//Before Destroyed, take falloff Damage
	Super::Destroyed();
}

void AProjectileGrenade::BeginPlay()
{
	AActor::BeginPlay();

	StartOwnerCollisionIgnoreWindow();

	//Start Destory Timer
	StartDestroyTimer();

	//Spawn Trail Smoke Effect
	SpawnTrailSystem();
	
	//Add Bounce Delegate,OnBounce will be called when bounce occurs.
	ProjectileMovement->OnProjectileBounce.AddDynamic(this, &AProjectileGrenade::OnBounce);
}

void AProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BounceSound, GetActorLocation());
	}
}
