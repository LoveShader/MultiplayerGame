// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

class UAnnouncement;
class UCharacterOverlay;
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
	virtual void ReceivedPlayer() override; // Sync with server clock as soon as possible
	virtual float GetServerTime() const;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
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
	void HandleMatchHasStarted();
	void OnMatchStateSet(FName State);
protected:
	virtual void BeginPlay() override;
	void SetHUDTime();

	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();
	
	UFUNCTION(Client, Reliable)
	void ClientJoinMidgame(FName MatchOfState, float Warm, float Match, float CoolDown, float LevelStartTime);

	void UpdateHUDWarmupCountdown(float CountdownTime);

	void PollInit();
private:
	UPROPERTY()
	ABlasterHUD* BlasterHUD;

	UPROPERTY()
	UCharacterOverlay* BlasterOverlay;

	UPROPERTY()
	UAnnouncement* BlasterAnnouncement;

	float MatchTime = 0.0f;
	float WarmupTime = 0.0f;
	float CoolDownTime = 0.0f;
	float LevelStartingTime = 0.0f;

	uint32 CountdownInt = 0;

	float ClientServerDelta = 0.f; // difference between client and server time

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;
	void CheckTimeSync(float DeltaTime);
	void HandleCoolDown();

	UPROPERTY(ReplicatedUsing=OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	float HUDHealth;
	float HUDMaxHealth;
	float HUDScore;
	int32 HUDDefeats;
};
