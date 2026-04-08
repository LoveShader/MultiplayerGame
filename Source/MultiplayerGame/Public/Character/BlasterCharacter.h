// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Components/TimelineComponent.h"
#include "Interface/AmmoReceiverInterface.h"
#include "Interface/ICombatUIProvider.h"
#include "Interface/InteractWithCrosshairsInterface.h"
#include "MultiplayerGame/BlasterTypes/CombatState.h"
#include "MultiplayerGame/BlasterTypes/TurnInPlace.h"
#include "BlasterCharacter.generated.h"

class ABlasterPlayerState;
class ABlasterPlayerController;
class UCombatComponent;
class UBuffComponent;
class ULagCompensationComponent;
class AWeapon;
class UWidgetComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;

UCLASS()
class MULTIPLAYERGAME_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface, public IICombatUIProvider, public IAmmoReceiverInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	friend class UBuffComponent;
	void UpdateHUDHealth();
	void UpdateHUDShield();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	void PlayFireMontage(bool bIsAiming);
	void PlayReloadMontage();
	void PlayThrowGrenadeMontage();

	/**
	 * Implement Pickup Interface
	 */
	virtual bool CanReceiveAmmo(EWeaponType WeaponType) const override;
	virtual void ReceiveAmmo(EWeaponType WeaponType, int32 AmmoAmount) override;
	
	void Elim();
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastElim();

	virtual void Destroyed() override;

	virtual UCombatComponent* GetCombatComponentForUI() override;
	ECombatState GetCombatState() const;

	void OnRep_PlayerState() override;
	void PossessedBy(AController* NewController) override;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	virtual void OnRep_ReplicateMovement() override;
protected:
	virtual void BeginPlay() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void CalculateAO_Pitch();
	float CalculateSpeed() const;
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();
	virtual void Jump() override;
	void FireButtonPressed();
	void FireButtonReleased();
	void ReloadButtonPressed();
	void ThrowGrenadeButtonPressed();
	
	void PlayHitReactMontage();
	void PlayElimMontage();
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser );
private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	UCameraComponent* FollowCamera;

	//Input Action And Input Mapping Context
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* LookAction;
	
	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* EquipAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* AimAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* FireAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* ThrowGrenadeAction;
	
	UPROPERTY(EditAnywhere, Category = Input)
	UInputMappingContext* InputContext;

	/* Montage Section */
	UPROPERTY(EditAnywhere, Category = Montage)
	UAnimMontage* FireMontage;

	UPROPERTY(EditAnywhere, Category = Montage)
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = Montage)
	UAnimMontage* ThrowGrenadeMontage;

	UPROPERTY(EditAnywhere, Category = Montage)
	UAnimMontage* ElimMontage;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappedWeapon)
	AWeapon* OverlappedWeapon;

	UFUNCTION()
	void OnRep_OverlappedWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	UBuffComponent* Buff;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	ULagCompensationComponent* LagCompensation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	class UWidgetComponent* OverheadWidget;
	
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();
	
	/* Calculate AimOffset yaw and pitch */
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	/* Simulate Proxy Rotation */
	UPROPERTY(EditAnywhere)
	float TurnThreshold = 0.5f;

	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

	/* Turning In Place State */
	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;
	
	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere, Category = Montage)
	UAnimMontage* HitReactMontage;

	/**
	 * Health Property
	 */
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.0f;
	
	UPROPERTY(ReplicatedUsing=OnRep_Health)
	float Health = 100.0f;
	
	UFUNCTION()
	void OnRep_Health(float LastHealth);

	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Shield, Category = "Player Stats")
	float Shield = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 100.0f;

	UFUNCTION()
	void OnRep_Shield(float LastShield);

	/**
	 * Add PlayerController, Use it to controll the character's health
	 */
	UPROPERTY(VisibleAnywhere)
	ABlasterPlayerController* BlasterPlayerController;

	bool bIsElimed;
	
	UPROPERTY(EditDefaultsOnly, Category = "Timer Properties")
	float ElimDelay = 3.5f;
	
	FTimerHandle ElimTimer;

	void ElimTimerFinished();

	/**
	 * Dissolve Effect
	 */
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeLine;
	FOnTimelineFloat DissolveTrack;		//这个是delegate，与蓝图中的track是一回事

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;
	
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();		//To start timeLine,also need bind update function and our dissolve track

	// Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	// Material instance set on the blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

	/**
	 * Elim Bot Effects
	 */
	UPROPERTY(EditAnywhere, Category = Elim)
	UParticleSystem* ElimBotParticle;

	UPROPERTY(EditAnywhere, Category = Elim)
	USoundBase* ElimBotSound;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotComponent;
	
	UPROPERTY()
	ABlasterPlayerState* BlasterPlayerState;
	//for init playerState and update score for the player first time spawn
	void PollInit();
	void PollInitInput();

	bool bInputsSet;
	bool bRotateRootBone = false;

	void ShowElimTextIfLocallyControlled();
	void DropOrDestroyWeapons();
	void ClearWeaponTypeText();
	void ClearCarriedAmmoText();

	/**
	 * Grenade
	 */
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;

	void SpawnDefaultWeapon() const;

	/**
	 * Hit boxes used for server-side rewind
	 */
	UPROPERTY(EditAnywhere)
	class UBoxComponent* head;

	UPROPERTY(EditAnywhere)
	UBoxComponent* pelvis;

	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_02;

	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_03;

	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* backpack;

	UPROPERTY(EditAnywhere)
	UBoxComponent* blanket;

	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_r;

	UPROPERTY()
	TMap<FName, UBoxComponent*> HitCollisionBoxes;
public:
	UPROPERTY(Replicated)
	bool bDisableGameplay = false;
public:
	void SetOverlappedWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped() const;
	bool IsAiming() const;
	bool IsLocallyReloading() const;
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon() const;
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool GetIsElimed() const { return bIsElimed; }
	FORCEINLINE	float GetHealth() const { return Health; }
	FORCEINLINE	float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	FORCEINLINE UCombatComponent* GetCombat() const {return Combat;}
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }
	FORCEINLINE bool GetRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const {return AttachedGrenade;}
	FORCEINLINE const TMap<FName, UBoxComponent*>& GetHitCollisionBoxes() const { return HitCollisionBoxes; }
};
