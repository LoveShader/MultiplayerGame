// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/BlasterPlayerController.h"

#include "BlasterComponents/CombatComponent.h"
#include "Character/BlasterCharacter.h"
#include "Character/CharacterOverlay.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameMode.h"
#include "GameMode/BlasterGameMode.h"
#include "HUD/Announcement.h"
#include "HUD/BlasterHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "PlayerState/BlasterPlayerState.h"
#include "Weapon/Weapon.h"



void ABlasterPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SetHUDTime();
	CheckTimeSync(DeltaSeconds);
	CheckPing(DeltaSeconds);
	PollInit();
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
	} else
	{
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
		bInitializeHealth = true;
	}
}

void ABlasterPlayerController::UpdateHUDShield(float Shield, float MaxShield)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->ShieldBar &&
		BlasterHUD->CharacterOverlay->ShieldText;

	if (bHudValid)
	{
		const float Percent = Shield / MaxShield;
		BlasterHUD->CharacterOverlay->ShieldBar->SetPercent(Percent);
		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::RoundToInt(Shield), FMath::RoundToInt(MaxShield));
		BlasterHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	} else
	{
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
		bInitializeShield = true;
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
	} else
	{
		HUDScore = Score;
		bInitializeScore = true;
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
	} else
	{
		HUDDefeats = Defeats;
		bInitializeDefeats = true;
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
	} else
	{
		bInitializeAmmo = true;
		HUDAmmo = WeaponAmmo;
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
	} else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = CarriedAmmo;
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
		if (CountdownTime < 0.f)
		{
			BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}
		FString MatchCountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(MatchCountdownText));
	}
}

void ABlasterPlayerController::SetElimTextVisibility(bool bVisibility)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
			BlasterHUD->CharacterOverlay->ElimText;

	if (bHudValid)
	{
		ESlateVisibility Visibility = bVisibility ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
		BlasterHUD->CharacterOverlay->ElimText->SetVisibility(Visibility);
	}
}

void ABlasterPlayerController::UpdateHUDWarmupCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	int32 Minutes = FMath::FloorToInt(CountdownTime / 60.0f);
	int32 Seconds = CountdownTime - Minutes * 60;
	
	bool bHudValid = BlasterHUD &&
		BlasterHUD->Announcement &&
		BlasterHUD->Announcement->WarmupTime;
	if (bHudValid)
	{
		if (CountdownTime < 0.0f)
		{
			BlasterHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		FString WarmupCountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString(WarmupCountdownText));
	}
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn))
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
		UpdateHUDShield(BlasterCharacter->GetShield(), BlasterCharacter->GetMaxShield());
		if (BlasterCharacter->GetCombat())
		{
			UpdateGrenadeAmount(BlasterCharacter->GetCombat()->GetGrenades());
		}
		if (BlasterCharacter->GetEquippedWeapon())
		{
			//When Pawn is restart, it will execute after character BeginPlay, so use this update Ammo, CarriedAmmo and WeaponType
			UpdateHUDWeaponAmmo(BlasterCharacter->GetEquippedWeapon()->GetWeaponAmmo());
			UpdateHUDCarriedAmmo(BlasterCharacter->GetCombat()->GetCarriedAmmo());
			UpdateHUDWeaponType(BlasterCharacter->GetCombat()->GetWeaponTypeText());
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
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHudValid)
	{
		FString WeaponAmmoText = FString::Printf(TEXT("%d"), 0);
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(WeaponAmmoText));
	} 
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		if (BlasterHUD->CharacterOverlay == nullptr)
			BlasterHUD->AddCharacterOverlay();
		if (BlasterHUD->Announcement)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCoolDown();
	}
}

void ABlasterPlayerController::StartShowElimTextTimer()
{
	SetElimTextVisibility(true);
	GetWorldTimerManager().SetTimer(
		ElimTextTimer,
		this,
		&ThisClass::ElimTextTimerFinished,
		ShowElimTextTime
	);
}

void ABlasterPlayerController::UpdateHUDWeaponType(const FString& WeaponType)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
			BlasterHUD->CharacterOverlay->WeaponType;

	if (bHudValid)
	{
		BlasterHUD->CharacterOverlay->WeaponType->SetText(FText::FromString(WeaponType));
	} else
	{
		bInitializeWeaponType = true;
		HUDWeaponType = WeaponType;
	}
}

void ABlasterPlayerController::UpdateGrenadeAmount(int32 GrenadeAmount)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
			BlasterHUD->CharacterOverlay->GrenadeAmount;

	if (bHudValid)
	{
		BlasterHUD->CharacterOverlay->GrenadeAmount->SetText(FText::AsNumber(GrenadeAmount));
	} else
	{
		HUDGrenadeAmount = GrenadeAmount;
		bInitializeGrenadeAmount = true;
	}
}

void ABlasterPlayerController::ElimTextTimerFinished()
{
	SetElimTextVisibility(false);
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

	ServerCheckMatchState();
}

void ABlasterPlayerController::SetHUDTime()
{
	float TimeLeft = 0.0f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress)	TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown)	TimeLeft = WarmupTime + MatchTime + CoolDownTime - GetServerTime() + LevelStartingTime;
	
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
	if (SecondsLeft != CountdownInt)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
			UpdateHUDWarmupCountdown(TimeLeft);
		if (MatchState == MatchState::InProgress)
			UpdateHUDMatchCountdown(TimeLeft);
	}
	CountdownInt = SecondsLeft;
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		MatchState = GameMode->GetMatchState();
		CoolDownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;

		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CoolDownTime, LevelStartingTime);	
	}
}

void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName MatchOfState, float Warm, float Match, float CoolDown,
	float LevelStartTime)
{
	MatchState = MatchOfState;
	WarmupTime = Warm;
	MatchTime = Match;
	CoolDownTime = CoolDown;
	LevelStartingTime = LevelStartTime;

	OnMatchStateSet(MatchState);

	//Add BlasterHUD in Listen Server Controller
	if (HasAuthority())
	{
		BlasterHUD = (BlasterHUD == nullptr) ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	}

	if (BlasterHUD && MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
	}
}

void ABlasterPlayerController::PollInit()
{
	if (BlasterOverlay == nullptr)
	{
		BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
		if (BlasterHUD && BlasterHUD->CharacterOverlay)
		{
			BlasterOverlay = BlasterHUD->CharacterOverlay;
			if (bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
			if (bInitializeShield) UpdateHUDShield(HUDShield, HUDMaxShield);
			if (bInitializeScore) UpdateHUDScore(HUDScore);
			if (bInitializeDefeats) UpdateHUDDefeats(HUDDefeats);
			SetElimTextVisibility(false);
			if (bInitializeWeaponType) UpdateHUDWeaponType(HUDWeaponType);
			if (bInitializeAmmo)  UpdateHUDWeaponAmmo(HUDAmmo);
			if (bInitializeCarriedAmmo) UpdateHUDCarriedAmmo(HUDCarriedAmmo);
			StopHighPingWarning();

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
			if (BlasterCharacter && BlasterCharacter->GetCombat() && bInitializeGrenadeAmount)
			{
				UpdateGrenadeAmount(BlasterCharacter->GetCombat()->GetGrenades());
			}
		}
	}
}

void ABlasterPlayerController::HighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HighPingImage &&
			BlasterHUD->CharacterOverlay->HighPingAnimation;
	if (bHudValid)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(1.0f);
		BlasterHUD->CharacterOverlay->PlayAnimation(BlasterHUD->CharacterOverlay->HighPingAnimation,
			0.0f,
			5);
	}
}

void ABlasterPlayerController::StopHighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HighPingImage &&
			BlasterHUD->CharacterOverlay->HighPingAnimation;
	if (bHudValid)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnimation))
		{
			BlasterHUD->CharacterOverlay->StopAnimation(BlasterHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

void ABlasterPlayerController::CheckPing(float DeltaTime)
{
	if (!IsLocalController())
	{
		return;
	}

	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime >= CheckPingFrequency)
	{
		if (APlayerState* BlasterPlayerState = GetPlayerState<APlayerState>())
		{
			const float Ping = BlasterPlayerState->GetPingInMilliseconds();
			if (Ping > HighPingThreshold)
			{
				PingAnimationRunningTime = 0.f;
				HighPingWarning();
			}
		}
		HighPingRunningTime = 0.f;
	}

	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	const bool bAnimationPlaying = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HighPingAnimation && 
		BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnimation);

	if (bAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaTime;
		if (PingAnimationRunningTime >= HighPingDuration)
		{
			PingAnimationRunningTime = 0.f;
			StopHighPingWarning();
		}
	}
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

void ABlasterPlayerController::HandleCoolDown()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (!BlasterHUD)	return;
	
	BlasterHUD->CharacterOverlay->RemoveFromParent();
	bool bHUDValid = BlasterHUD->Announcement &&
		             BlasterHUD->Announcement->AnnouncementText &&
		             	BlasterHUD->Announcement->InfoText;
	
	if (bHUDValid)
	{
		BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
		FString AnnouncementText("New Match Starts In:");
		BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));
		BlasterHUD->Announcement->InfoText->SetText(FText());
	}

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter && BlasterCharacter->GetCombat())
	{
		BlasterCharacter->bDisableGameplay = true;
		BlasterCharacter->GetCombat()->FireButtonPressed(false);
	}
}


void ABlasterPlayerController::OnRep_MatchState()
{
	if (MatchState == ::MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCoolDown();
	}
}

float ABlasterPlayerController::GetServerTime() const
{
	if (HasAuthority())	return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

float ABlasterPlayerController::GetSingleTripTime() const
{
	return SingleTripTime;
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABlasterPlayerController, MatchState);
}

void ABlasterPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	//Update GrenadeAmount In Clients
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
	{
		UpdateHUDShield(BlasterCharacter->GetShield(), BlasterCharacter->GetMaxShield());
		if (BlasterCharacter->GetCombat())
		{
			UpdateGrenadeAmount(BlasterCharacter->GetCombat()->GetGrenades());
		}
	}
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
                                                                     float TimeServerReceivedClientRequest)
{
	//when server down to the client, server time is T1 + RTT / 2, and client time is GetWorld()->GetTimeSeconds.
	//so the two deltaTime = T1 + RTT/2 - CurrentClientTime
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = 0.5f * RoundTripTime;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	//TimeOfClientRequest is the Client Time T0
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();	//ServerTime of T1
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}
