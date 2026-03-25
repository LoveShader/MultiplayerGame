#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"

class ABlasterCharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MULTIPLAYERGAME_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuffComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void Heal(float HealAmount, float HealingTime);
	void BuffSpeed(float BaseSpeed, float CrouchSpeed, float BuffTime);
	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed);
	void BuffJump(float JumpVelocity, float BuffTime);
	void SetInitialJumpVelocity(float JumpVelocity);

protected:
	virtual void BeginPlay() override;

private:
	friend class ABlasterCharacter;

	UPROPERTY()
	ABlasterCharacter* Character;

	bool bHeal = false;

	float AmountToHeal = 0.0f;

	float HealingRate = 0.0f;

	void HealRampup(float DeltaTime);

	float InitialBaseSpeed = 0.0f;

	float InitialCrouchSpeed = 0.0f;

	FTimerHandle SpeedBuffTimer;

	FTimerHandle JumpBuffTimer;

	void ResetSpeeds();

	float InitialJumpVelocity = 0.0f;

	void ResetJumpVelocity();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpVelocity);
};
