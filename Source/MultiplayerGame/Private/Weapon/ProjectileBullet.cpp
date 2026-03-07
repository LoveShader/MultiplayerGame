// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileBullet.h"

#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	//Set Projectile Rotation with it's Velocity
	ProjectileMovement->SetIsReplicated(true);
	ProjectileMovement->bRotationFollowsVelocity = true;
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		AController* OwnerController = OwnerCharacter->GetController();
		if (OwnerController)
		{
			UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
		}
	}
	//because the parent class will destroy the actor in OnHit function, we need call it last
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
