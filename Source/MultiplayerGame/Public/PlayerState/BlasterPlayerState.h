// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreChanged, float, NewScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDefeatsChanged, int32, NewDefeats);

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

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnScoreChanged OnScoreChanged;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnDefeatsChanged OnDefeatsChanged;
private:
	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats = 0;

	UFUNCTION()
	void OnRep_Defeats();
};
