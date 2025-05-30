// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/BlasterPlayerState.h"

#include "Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "PlayerController/BlasterPlayerController.h"

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	SetPlayerScore(GetScore());
}

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABlasterPlayerState, Defeats);
}

ABlasterPlayerController* ABlasterPlayerState::GetController()
{
	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
	}
	return Controller;
}

void ABlasterPlayerState::SetPlayerScore(float NewScore)
{
	ABlasterPlayerController* PlayerController = GetController();
	if (PlayerController)
	{
		PlayerController->SetHUDScore(NewScore);
	}
}

void ABlasterPlayerState::SetPlayerDefeats(int32 NewDefeats)
{
	ABlasterPlayerController* PlayerController = GetController();
	if (PlayerController)
	{
		PlayerController->SetHUDDefeats(NewDefeats);
	}
}

void ABlasterPlayerState::OnRep_Defeats()
{
	SetPlayerDefeats(Defeats);
}

void ABlasterPlayerState::AddScore(float ScoreAmount)
{
	const float NewScore = GetScore() + ScoreAmount;
	SetScore(NewScore);

	SetPlayerScore(NewScore);
}

void ABlasterPlayerState::AddDefeats(int DefeatAmount)
{
	Defeats += DefeatAmount;
	
	SetPlayerDefeats(Defeats);
}
