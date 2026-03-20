// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class USoundCue;
class UProjectileMovementComponent;
class UBoxComponent;
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class MULTIPLAYERGAME_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	void PlayHitEffects();
	void ExplodeDamage();
	void SpawnTrailSystem();
	void StartDestroyTimer();
	void DestroyTimerFinished();
	void IgnoreOwnerCollision(bool bShouldIgnore);
	void ReEnableOwnerCollision();

	UFUNCTION(Server, Reliable)
	void ServerHitEffects();

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastHitEffects();

	UPROPERTY(EditDefaultsOnly)
	float Damage = 20.0f;

	UPROPERTY(EditAnywhere)
	UBoxComponent* CollisionBox;
	
	//Smoke Trail  
	UPROPERTY()
	UNiagaraComponent* TrailComponent;

	UPROPERTY(VisibleDefaultsOnly)
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovement;
private:
	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer;

	UPROPERTY()
	class UParticleSystemComponent* TracerComponent;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticle;

	UPROPERTY(EditAnywhere)
	USoundCue* ImpactSound;

	UPROPERTY(EditDefaultsOnly)
	float MinDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly)
	float MinInnerRadius = 200.0f;

	UPROPERTY(EditDefaultsOnly)
	float MaxOuterRadius = 500.0f;

	//Smoke Trail 
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* TrailSystem;

	/**
	 * Delay Destory Timer
	 */
	FTimerHandle DestroyTimer;

	FTimerHandle OwnerCollisionTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.0f;

	UPROPERTY(EditDefaultsOnly)
	float OwnerIgnoreTime = 0.15f;
};
