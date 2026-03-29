// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HUD/BlasterHUD.h"
#include "MultiplayerGame/BlasterTypes/CombatState.h"
#include "Weapon/WeaponTypes.h"
#include "CombatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCarriedAmmoChanged, int32, NewAmmo);

USTRUCT(BlueprintType)
struct FCarriedAmmoConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

	UPROPERTY(EditAnywhere)
	int32 InitialCarriedAmmo;

	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 0;
};

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
	void ThrowGrenade();
	UFUNCTION(BlueprintCallable)
	void FinishReloading();
	UFUNCTION(BlueprintCallable)
	void FinishThrowGrenade();
	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();
	void FireButtonPressed(bool bPressed);

	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

	/**
	 * Pickup Ammo implementation
	 */
	bool CanPickupAmmo(EWeaponType WeaponType) const;
	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);

	/**
	 * Swap Weapon
	 */
	UFUNCTION(Server, Reliable)
	void SwapWeapons();
protected:
	virtual void BeginPlay() override;
	void EquipWeapon(AWeapon* WeaponToEquip);
	void DroppedWeapon();
	void SetAiming(bool bAiming);
	ABlasterPlayerController* GetBlasterPlayerController();
	void UpdateCurrentWeaponTypeInHUD();
	void UpdateCurrentCarriedAmmoInHUD();

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon(AWeapon* LastWeapon);

	UFUNCTION()
	void OnRep_SecondaryWeapon();

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

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	void HandleReload();
	void HandleThrowGrenade();
	void UpdateAmmoValues();
	void UpdateCarriedAmmoUI();
	bool CanThrowGrenade() const;

private:
	UPROPERTY()
	ABlasterCharacter* Character;

	UPROPERTY()
	ABlasterPlayerController* PlayerController;

	UPROPERTY()
	ABlasterHUD* HUD;

	UPROPERTY(ReplicatedUsing=OnRep_EquippedWeapon, VisibleAnywhere)
	AWeapon* EquippedWeapon;

	UPROPERTY(ReplicatedUsing=OnRep_SecondaryWeapon, VisibleAnywhere)
	AWeapon* SecondaryWeapon;

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
	
	UPROPERTY(EditDefaultsOnly, Category = CarriedAmmos)
	TArray<FCarriedAmmoConfig> CarriedAmmoConfigs;
	
	UFUNCTION()
	void OnRep_CarriedAmmo();

	TMap<EWeaponType, int32> CarriedAmmoMap;

	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	UFUNCTION()
	void OnRep_ShotgunEndShellCount();

	int32 AmountToReload();

	void PlayEquipWeaponSound(AWeapon* WeaponToPlay);

	FString GetWeaponTypeDisplayName(EWeaponType WeaponType);
	void DropWeaponIfEquiped();
	void DropOrDestroyWeapon(AWeapon* Weapon);
	void DropOrDestroyWeapons();
	void SetCarriedAmmoFromCarriedAmmoMap();
	void ReloadEmptyWeapon();
	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);
	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);

	void UpdateShotgunAmmoValues();
	void ResetShotgunReloadTracking();
	void TryJumpToShotgunEnd();

	void JumpToShotgunEnd();

	UPROPERTY(ReplicatedUsing = OnRep_ShotgunEndShellCount)
	int32 ShotgunEndShellCount = 0;

	int32 LocalShotgunShellCount = 0;
	bool bShotgunEndConsumed = false;

	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);
	void AttachActorToBackpack(AActor* ActorToAttach);

	void AttachActorToSocket(AActor* ActorToAttach, FName SocketName);

	void ShowAttachedGrenade(bool bShowGrenade);

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> GrenadeClass;

	UFUNCTION(Server, Reliable)
	void ServerSpawnGrenade(const FVector_NetQuantize& HitLocation);

	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Grenades)
	int32 Grenades = 4;

	UPROPERTY(EditAnywhere)
	int32 MaxGrenades = 4;

	UFUNCTION()
	void OnRep_Grenades();

	int32 GetWeaponMaxCarriedAmmo(EWeaponType WeaponType) const;
public:
	/**
	 * Getter and Setter Function
	 */
	FORCEINLINE int32 GetCarriedAmmo() const {return CarriedAmmo;};
	FORCEINLINE ECombatState GetCombatState() const { return CombatState; }
	FORCEINLINE int32 GetGrenades() const { return Grenades; }
	FORCEINLINE AWeapon* GetSecondaryWeapon() const { return SecondaryWeapon; }
	bool CanSwapWeapon() const;
	FString GetWeaponTypeText();
};
