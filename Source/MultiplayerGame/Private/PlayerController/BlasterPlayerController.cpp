// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/BlasterPlayerController.h"

#include "BlasterComponents/CombatComponent.h"
#include "Character/BlasterCharacter.h"
#include "Character/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "HUD/BlasterHUD.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerState/BlasterPlayerState.h"
#include "Weapon/Weapon.h"



void ABlasterPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SetHUDTime();
	CheckTimeSync(DeltaSeconds);
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	/**
	 * 1. Get the Blaster HUD
	 * 2. check progress bar and TextBlock is not nullptr
	 * 3. Update the health bar and health text
	 */

	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HealthBar &&
		BlasterHUD->CharacterOverlay->HealthText;
	
	if (bHudValid)
	{
		const float Percent = Health / MaxHealth;
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(Percent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}

void ABlasterPlayerController::UpdateHUDScore(float Score)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->ScoreAmount;
	if (bHudValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
}

void ABlasterPlayerController::UpdateHUDDefeats(int32 Defeats)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->DefeatsAmount;
	if (bHudValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
}

void ABlasterPlayerController::UpdateHUDWeaponAmmo(int32 WeaponAmmo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHudValid)
	{
		FString WeaponAmmoText = FString::Printf(TEXT("%d"), WeaponAmmo);
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(WeaponAmmoText));
	}
}

void ABlasterPlayerController::UpdateHUDCarriedAmmo(int32 CarriedAmmo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHudValid)
	{
		FString CarriedAmmoText = FString::Printf(TEXT("%d"), CarriedAmmo);
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(CarriedAmmoText));
	}
}

void ABlasterPlayerController::UpdateHUDMatchCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	int32 Minutes = FMath::FloorToInt(CountdownTime / 60.0f);
	int32 Seconds = CountdownTime - Minutes * 60;
	
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->MatchCountdownText;
	if (bHudValid)
	{
		FString MatchCountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(MatchCountdownText));
	}
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn))
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
		if (BlasterCharacter->GetEquippedWeapon())
		{
			UpdateHUDWeaponAmmo(BlasterCharacter->GetEquippedWeapon()->GetWeaponAmmo());
		}
		else
		{
			ClearWeaponAmmoHUD();
		}
	}
	
}

void ABlasterPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	if (ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>())
	{
		BlasterPlayerState->OnScoreChanged.AddDynamic(this, &ABlasterPlayerController::UpdateHUDScore);
		BlasterPlayerState->OnDefeatsChanged.AddDynamic(this, &ABlasterPlayerController::UpdateHUDDefeats);
	}
}

void ABlasterPlayerController::ClearWeaponAmmoHUD()
{
	UpdateHUDWeaponAmmo(0);
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>())
		{
			BlasterPlayerState->OnScoreChanged.AddDynamic(this, &ABlasterPlayerController::UpdateHUDScore);
			BlasterPlayerState->OnDefeatsChanged.AddDynamic(this, &ABlasterPlayerController::UpdateHUDDefeats);
		}
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	int32 SecondsLeft = FMath::FloorToInt(MatchTime - GetServerTime());
	if (SecondsLeft != CountdownInt)
	{
		UpdateHUDMatchCountdown(MatchTime - GetServerTime());
	}
	CountdownInt = SecondsLeft;
}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime -= TimeSyncFrequency;
	}
}

float ABlasterPlayerController::GetServerTime() const
{
	if (HasAuthority())	return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
                                                                     float TimeServerReceivedClientRequest)
{
	//when server down to the client, server time is T1 + RTT / 2, and client time is GetWorld()->GetTimeSeconds.
	//so the two deltaTime = T1 + RTT/2 - CurrentClientTime
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	//TimeOfClientRequest is the Client Time T0
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();	//ServerTime of T1
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}
