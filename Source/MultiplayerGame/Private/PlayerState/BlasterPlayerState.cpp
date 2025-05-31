// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/BlasterPlayerState.h"

#include "Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "PlayerController/BlasterPlayerController.h"

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	OnScoreChanged.Broadcast(GetScore());
}

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABlasterPlayerState, Defeats);
}

void ABlasterPlayerState::OnRep_Defeats()
{
	OnDefeatsChanged.Broadcast(Defeats);
}

void ABlasterPlayerState::AddScore(float ScoreAmount)
{
	const float NewScore = GetScore() + ScoreAmount;
	SetScore(NewScore);

	OnScoreChanged.Broadcast(NewScore);
}

void ABlasterPlayerState::AddDefeats(int DefeatAmount)
{
	Defeats += DefeatAmount;
	
	OnDefeatsChanged.Broadcast(Defeats);
}
