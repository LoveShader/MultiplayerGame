// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileRocket.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstanceController.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Weapon/RocketMovementComponent.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>("RocketMesh");
	RocketMesh->SetupAttachment(GetRootComponent());
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>("RocketMovementComponent");
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
}

void AProjectileRocket::Destroyed()
{
	
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);
	}
	
	if (TrailSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}

	if (ProjectileLoop && LoopingSoundAttenuation)
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.0f,
			1.0f,
			0.0f,
			LoopingSoundAttenuation,
			(USoundConcurrency*)nullptr,
			false
		);
	}
}	
	

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner())
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit Self"));
		return;
	}
	
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
	

	GetWorldTimerManager().SetTimer(DestroyTimer, this, &AProjectileRocket::DestroyTimerFinished, DestroyTime);

	PlayHitEffects();
	
	if (TrailComponent && TrailComponent->GetSystemInstanceController())
	{
		TrailComponent->GetSystemInstanceController()->Deactivate();
	}

	if (RocketMesh)
	{
		RocketMesh->SetVisibility(false);
		RocketMesh->SetHiddenInGame(true);
	}
	
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}
}

void AProjectileRocket::DestroyTimerFinished()
{
	Destroy();
}
