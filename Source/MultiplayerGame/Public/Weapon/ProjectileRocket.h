// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileRocket.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
/**
 * 
 */
UCLASS()
class MULTIPLAYERGAME_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()
public:
	AProjectileRocket();
	virtual void Destroyed() override;
protected:
	virtual void BeginPlay() override;
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

	//DestoryTimer Callback 
	void DestroyTimerFinished();
private:
	UPROPERTY(VisibleDefaultsOnly)
	UStaticMeshComponent* RocketMesh;
	
	UPROPERTY(EditDefaultsOnly)
	float MinDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly)
	float MinInnerRadius = 200.0f;

	UPROPERTY(EditDefaultsOnly)
	float MaxOuterRadius = 500.0f;
	/**
	 * Smoke Trail 
	 */
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* TrailSystem;

	UPROPERTY()
	UNiagaraComponent* TrailComponent;
	
	/**
	 * Smoke Trail Sound Effect
	 */
	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoop;

	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* LoopingSoundAttenuation;
	/**
	 * Delay Destory Timer
	 */
	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.0f;
};
