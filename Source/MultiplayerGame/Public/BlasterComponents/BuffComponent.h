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
};
