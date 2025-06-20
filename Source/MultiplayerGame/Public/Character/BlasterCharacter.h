// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Components/TimelineComponent.h"
#include "Interface/ICombatUIProvider.h"
#include "Interface/InteractWithCrosshairsInterface.h"
#include "MultiplayerGame/BlasterTypes/CombatState.h"
#include "MultiplayerGame/BlasterTypes/TurnInPlace.h"
#include "BlasterCharacter.generated.h"

class ABlasterPlayerState;
class ABlasterPlayerController;
class UCombatComponent;
class AWeapon;
class UWidgetComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;

UCLASS()
class MULTIPLAYERGAME_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface, public IICombatUIProvider
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	void UpdateHUDHealth();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	void PlayFireMontage(bool bIsAiming);
	void PlayReloadMontage();
	
	void Elim();
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastElim();

	virtual void Destroyed() override;

	virtual UCombatComponent* GetCombatComponentForUI() override;
	ECombatState GetCombatState() const;
protected:
	virtual void BeginPlay() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void AimOffset(float DeltaTime);
	virtual void Jump() override;
	void FireButtonPressed();
	void FireButtonReleased();
	void ReloadButtonPressed();
	
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
	UInputMappingContext* InputContext;

	/* Montage Section */
	UPROPERTY(EditAnywhere, Category = Montage)
	UAnimMontage* FireMontage;

	UPROPERTY(EditAnywhere, Category = Montage)
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = Montage)
	UAnimMontage* ElimMontage;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappedWeapon)
	AWeapon* OverlappedWeapon;

	UFUNCTION()
	void OnRep_OverlappedWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	UCombatComponent* Combat;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();
	
	/* Calculate AimOffset yaw and pitch */
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

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
	void OnRep_Health();

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
public:
	UPROPERTY(Replicated)
	bool bDisableGameplay = false;
public:
	void SetOverlappedWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped() const;
	bool IsAiming() const;
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon() const;
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool GetIsElimed() const { return bIsElimed; }
	FORCEINLINE	float GetHealth() const { return Health; }
	FORCEINLINE	float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE UCombatComponent* GetCombat() const {return Combat;}
};
