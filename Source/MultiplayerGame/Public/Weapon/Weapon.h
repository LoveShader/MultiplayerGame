// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAmmoChanged, int32, NewAmmo);

class USphereComponent;
class UWidgetComponent;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "EquippedSecondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),
	EWS_MAX UMETA(DisplayName = "DefaultMax")
};

UCLASS()
class MULTIPLAYERGAME_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	void ShowPickupWidget(bool bShowWidget);
	virtual void Fire(const FVector& HitTarget);
	FVector TraceEndWithScatter(const FVector& HitTarget) const;
	void BroadcastCurrentAmmo() const;
	void AddAmmo(int AmmoToReload);
	void SetWeaponOutlineStencil(EWeaponOutlineStencil NewOutlineStencil);
	void EnableWeaponOutline(bool bEnabled);
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION()
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ApplyWeaponState();
	void OnInitial();
	void OnEquipped();
	void OnEquippedSecondary();
	void OnDropped();
	void SetWeaponOutlineEnabled(bool bEnabled) const;
	FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget) const;

	UPROPERTY(EditAnywhere, Category = "WeaponDamage")
	float Damage = 20.0f;
private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USphereComponent* AreaSphere;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	UWidgetComponent* PickupWidget;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

	/** 
	* Zoomed FOV while aiming
	*/
	UPROPERTY(EditAnywhere, Category = "ZoomParameters")
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere, Category = "ZoomParameters")
	float ZoomInterpSpeed = 20.f;

	//Ammo
	UPROPERTY(EditAnywhere,Category = "AmmoParameters")
	int32 Ammo;

	//The Number of UnProcessed Server requests for Ammo
	//Increment in SpendRound, Decrement in ClientUpdateAmmo
	int32 Sequence = 0;
	
	UPROPERTY(EditAnywhere, Category = "AmmoParameters")
	int MagCapcity;

	void SpendRound();

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);

	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToReload);
	
	UPROPERTY(EditAnywhere, Category = "WeaponTypeParameters")
	EWeaponType WeaponType;

	UPROPERTY(EditAnywhere, Category = "WeaponTypeParameters")
	EFireType FireType = EFireType::EFT_HitScan;

	UPROPERTY(EditAnywhere, Category = "Weapon Outline")
	EWeaponOutlineStencil OutlineStencil = EWeaponOutlineStencil::EWOS_Blue;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;

	/** 
	* Automatic fire
	*/
	UPROPERTY(EditAnywhere, Category = "Combat|AutoFire")
	float FireDelay = 0.15f;
	
	UPROPERTY(EditAnywhere, Category = "Combat|AutoFire")
	bool bAutomatic = true;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class USoundCue* EquipSound;

	UPROPERTY(EditDefaultsOnly, Category = "Montage")
	FName ReloadMontageName;

	bool bDestroyWeapon = false;
public:
	/**
	 * Blaster CrossHair HUD Textures
	 */
	UPROPERTY(EditAnywhere, Category = "HUD Properties")
	class UTexture2D* CrossHairCenter;

	UPROPERTY(EditAnywhere, Category = "HUD Properties")
	UTexture2D* CrossHairLeft;
	
	UPROPERTY(EditAnywhere, Category = "HUD Properties")
	UTexture2D* CrossHairRight;
	
	UPROPERTY(EditAnywhere, Category = "HUD Properties")
	UTexture2D* CrossHairTop;

	UPROPERTY(EditAnywhere, Category = "HUD Properties")
	UTexture2D* CrossHairBottom;
	void SetWeaponState(EWeaponState State);

	void DroppedWeapon();
	FORCEINLINE USphereComponent* GetAreaSphere() const {return AreaSphere;}
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const {return WeaponMesh;}
	FORCEINLINE float GetZoomedFOV() const {return ZoomedFOV;}
	FORCEINLINE float GetZoomInterpSpeed() const {return ZoomInterpSpeed;}
	FORCEINLINE int32 GetWeaponAmmo() const {return Ammo;}
	FORCEINLINE int32 GetMagCapacity() const {return MagCapcity;}
	FORCEINLINE EWeaponType GetWeaponType() const {return WeaponType;}
	FORCEINLINE EFireType GetFireType() const { return FireType; }
	FORCEINLINE bool IsNeedReload() const {return Ammo < MagCapcity;}
	FORCEINLINE bool IsEmpty() const {return Ammo <= 0;}
	FORCEINLINE float GetFireDelay() const {return FireDelay;}
	FORCEINLINE bool GetAutomatic() const {return bAutomatic;}
	FORCEINLINE USoundCue* GetEquipSound() const {return EquipSound;}
	FORCEINLINE FName GetReloadMontageName() const {return ReloadMontageName;}
	FORCEINLINE bool IsFull() const {return Ammo == MagCapcity;}
	FORCEINLINE bool IsDestroyWeapon() const {return bDestroyWeapon;}
	FORCEINLINE void SetWeaponoDestroyed(bool bDestroyed) { bDestroyWeapon = bDestroyed;}
	FORCEINLINE EWeaponOutlineStencil GetWeaponOutlineStencil() const { return OutlineStencil; }
	FORCEINLINE bool IsUseScatter() const { return bUseScatter; }
	FORCEINLINE float GetDamage() const { return Damage; }
public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAmmoChanged OnAmmoChanged;
};
