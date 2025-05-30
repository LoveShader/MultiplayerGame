// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/BlasterPlayerState.h"

#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	SetPlayerScore(GetScore());
}

void ABlasterPlayerState::SetPlayerScore(float NewScore)
{
	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(NewScore);
		}
	}
}

void ABlasterPlayerState::AddScore(float ScoreAmount)
{
	const float NewScore = GetScore() + ScoreAmount;
	SetScore(NewScore);

	SetPlayerScore(NewScore);
}
