// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HUD/BlasterHUD.h"
#include "MultiplayerGame/BlasterTypes/CombatState.h"
#include "Weapon/WeaponTypes.h"
#include "CombatComponent.generated.h"

#define LINETRACE_LENGTH 80000.0f
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCarriedAmmoChanged, int32, NewAmmo);

class UCameraComponent;
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
	void Reload();
	UFUNCTION(BlueprintCallable)
	void FinishReloading();
	void FireButtonPressed(bool bPressed);
protected:
	virtual void BeginPlay() override;
	void EquipWeapon(AWeapon* WeaponToEquip);
	void DroppedWeapon();
	void SetAiming(bool bAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon(AWeapon* LastWeapon);

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	void Fire();

	void StartFireTimer();

	void FireTimerFinished();

	UFUNCTION()
	void OnWeaponAmmoChanged(int32 NewAmmo);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload();
	void UpdateAmmoValues();
	void UpdateCarriedAmmoUI();

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

	float CrosshairAimFactor;

	float CrosshairShootingFactor;

	float CrosshairTraceFactor;

	FHUDPackage HUDPackage;

	FVector HitTarget;

	float DefaultFOV;
	//use this value for interpolate FOV(from CurrentFOV to EquippedWeapon->ZoomedFOV)
	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.0f;

	void InterpFOV(float DeltaTime);
	/**
	 * Fire Timer Handle
	 */
	FTimerHandle FireTimerHandle;
	bool bCanFire;

	bool CanFire() const;

	// Carried Ammo for Current Weapon Type
	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30;

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 4;
	
	UFUNCTION()
	void OnRep_CarriedAmmo();

	TMap<EWeaponType, int32> CarriedAmmoMap;

	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	int32 AmountToReload();
public:
	/**
	 * Getter and Setter Function
	 */
	FORCEINLINE int32 GetCarriedAmmo() const {return CarriedAmmo;};
};
