// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

class ABlasterHUD;
/**
 * 
 */
UCLASS()
class MULTIPLAYERGAME_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaSeconds) override;
	void SetHUDHealth(float Health, float MaxHealth);
	UFUNCTION()
	void UpdateHUDScore(float Score);
	UFUNCTION()
	void UpdateHUDDefeats(int32 Defeats);
	UFUNCTION()
	void UpdateHUDWeaponAmmo(int32 WeaponAmmo);
	UFUNCTION()
	void UpdateHUDCarriedAmmo(int32 CarriedAmmo);

	void UpdateHUDMatchCountdown(float CountdownTime);
	
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_PlayerState() override;
	void ClearWeaponAmmoHUD();
protected:
	virtual void BeginPlay() override;
	void SetHUDTime();
private:
	UPROPERTY()
	ABlasterHUD* BlasterHUD;

	UPROPERTY(EditAnywhere)
	float MatchTime = 120.f;

	uint32 CountdownInt = 0;
};
