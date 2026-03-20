// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Projectile.h"

#include "NiagaraFunctionLibrary.h"
#include "Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MultiplayerGame/MultiplayerGame.h"
#include "Sound/SoundCue.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>("CollisionBox");
	SetRootComponent(CollisionBox);

	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECR_Block);
	
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}

	IgnoreOwnerCollision(true);
	GetWorldTimerManager().SetTimer(
		OwnerCollisionTimer,
		this,
		&AProjectile::ReEnableOwnerCollision,
		OwnerIgnoreTime
	);

	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
}

void AProjectile::IgnoreOwnerCollision(bool bShouldIgnore)
{
	if (!CollisionBox)
	{
		return;
	}

	if (AActor* ProjectileOwner = GetOwner())
	{
		CollisionBox->IgnoreActorWhenMoving(ProjectileOwner, bShouldIgnore);
	}

	if (APawn* ProjectileInstigator = GetInstigator())
	{
		CollisionBox->IgnoreActorWhenMoving(ProjectileInstigator, bShouldIgnore);
	}
}

void AProjectile::ReEnableOwnerCollision()
{
	IgnoreOwnerCollision(false);
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	ServerHitEffects();
	Destroy();
}

void AProjectile::PlayHitEffects()
{
	if (ImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticle, GetActorTransform());
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}

void AProjectile::ExplodeDamage()
{
	if (HasAuthority())
	{
		if (APawn* FirePawn = GetInstigator())
		{
			if (AController* FireController = FirePawn->GetController())
			{
				UGameplayStatics::ApplyRadialDamageWithFalloff(
					this,
					Damage,
					MinDamage,
					GetActorLocation(),
					MinInnerRadius,
					MaxOuterRadius,
					1.0f,
					UDamageType::StaticClass(),
					TArray<AActor*>(),
					this,
					FireController
					);
			}
		}
	}
}

void AProjectile::SpawnTrailSystem()
{
	if (TrailSystem)
	{
		TrailComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
}

void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(DestroyTimer, this, &AProjectile::DestroyTimerFinished, DestroyTime);
}

void AProjectile::DestroyTimerFinished()
{
	Destroy();
}

void AProjectile::NetMulticastHitEffects_Implementation()
{
	PlayHitEffects();
}

void AProjectile::ServerHitEffects_Implementation()
{
	NetMulticastHitEffects();	
}

void AProjectile::Destroyed()
{
	GetWorldTimerManager().ClearTimer(OwnerCollisionTimer);

	if (ImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticle, GetActorTransform());
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	Super::Destroyed();
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
