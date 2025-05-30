// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

class ABlasterPlayerController;
/**
 * 
 */
UCLASS()
class MULTIPLAYERGAME_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	virtual void OnRep_Score() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	void AddScore(float ScoreAmount);
	void AddDefeats(int DefeatAmount);
private:
	class ABlasterCharacter* Character;
	class ABlasterPlayerController* Controller;

	//helper Function
	void SetPlayerScore(float NewScore);
	void SetPlayerDefeats(int32 NewDefeats);
	ABlasterPlayerController* GetController();

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats = 0;

	UFUNCTION()
	void OnRep_Defeats();
};
