// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

#define LINETRACE_LENGTH 80000.0f

class ABlasterHUD;
class ABlasterPlayerController;
class ABlasterCharacter;
class AWeapon;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MULTIPLAYERGAME_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	friend class ABlasterCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void BeginPlay() override;
	void EquipWeapon(AWeapon* WeaponToEquip);
	void SetAiming(bool bAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void FireButtonPressed(bool bPressed);

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);
private:
	ABlasterCharacter* Character;

	ABlasterPlayerController* PlayerController;

	ABlasterHUD* HUD;

	UPROPERTY(ReplicatedUsing=OnRep_EquippedWeapon, VisibleAnywhere)
	AWeapon* EquippedWeapon;

	UPROPERTY(Replicated, VisibleAnywhere)
	bool bIsAiming;

	/* Unequipped Max Speed and Aim Max Walk Speed*/
	UPROPERTY(EditAnywhere, Category = Movement)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere, Category = Movement)
	float AimWalkSpeed;

	bool bFireButtonPressed;

	float CrosshairVelocityFactor;

	float CrosshairInAirFactor;

	FVector HitTarget;
};
