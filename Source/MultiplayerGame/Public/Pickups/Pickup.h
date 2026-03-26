// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

class USphereComponent;
class USoundCue;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class MULTIPLAYERGAME_API APickup : public AActor
{
	GENERATED_BODY()
	
public:	
	APickup();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
protected:
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere)
	UNiagaraComponent* PickupEffectComponent;

	UPROPERTY(EditAnywhere, Category = Particles)
	UNiagaraSystem* PickupEffect;
private:
	void BindOverlapTimerFinished();

	UPROPERTY(VisibleAnywhere)
	USphereComponent* OverlapSphere;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* PickupMesh;

	UPROPERTY(EditAnywhere)
	USoundCue* PickupSound;

	UPROPERTY(EditDefaultsOnly)
	float BaseRotationRate = 45.0f;

	UPROPERTY(EditDefaultsOnly, Category = Pickup)
	float BindOverlapTime = 0.25f;

	FTimerHandle BindOverlapTimer;
public:	

};
