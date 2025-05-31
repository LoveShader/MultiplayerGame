// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/BlasterPlayerController.h"

#include "Character/BlasterCharacter.h"
#include "Character/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "HUD/BlasterHUD.h"
#include "PlayerState/BlasterPlayerState.h"

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

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn))
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
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
