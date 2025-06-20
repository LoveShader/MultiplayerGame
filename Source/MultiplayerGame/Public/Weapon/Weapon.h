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
	void BroadcastCurrentAmmo() const;
	void AddAmmo(int AmmoToReload);
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION()
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
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
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	//Ammo
	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Ammo)
	int32 Ammo;

	UFUNCTION()
	void OnRep_Ammo();

	UPROPERTY(EditAnywhere)
	int MagCapcity;

	void SpendRound();

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

	/** 
	* Automatic fire
	*/
	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = 0.15f;
	
	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutomatic = true;
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
	FORCEINLINE bool IsNeedReload() const {return Ammo < MagCapcity;}
	FORCEINLINE bool IsEmpty() const {return Ammo <= 0;}
	FORCEINLINE float GetFireDelay() const {return FireDelay;}
	FORCEINLINE bool GetAutomatic() const {return bAutomatic;}
public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAmmoChanged OnAmmoChanged;
};
