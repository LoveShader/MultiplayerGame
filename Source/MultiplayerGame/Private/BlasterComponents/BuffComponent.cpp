#include "BlasterComponents/BuffComponent.h"

#include "Character/BlasterCharacter.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->HasAuthority())
	{
		HealRampup(DeltaTime);
	}
}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	if (Character == nullptr || HealAmount <= 0.0f)
	{
		return;
	}

	if (Character->Health >= Character->MaxHealth)
	{
		bHeal = false;
		AmountToHeal = 0.0f;
		HealingRate = 0.0f;
		return;
	}

	bHeal = true;
	AmountToHeal += HealAmount;
	HealingRate = AmountToHeal / FMath::Max(HealingTime, KINDA_SMALL_NUMBER);
}

void UBuffComponent::HealRampup(float DeltaTime)
{
	if (!bHeal || Character == nullptr)
	{
		return;
	}

	const float HealthBeforeHealing = Character->Health;
	Character->Health = FMath::Clamp(Character->Health + HealingRate * DeltaTime, 0.0f, Character->MaxHealth);
	const float HealThisFrame = Character->Health - HealthBeforeHealing;
	AmountToHeal -= HealThisFrame;

	if (Character->IsLocallyControlled())
	{
		Character->UpdateHUDHealth();
	}

	if (AmountToHeal <= 0.0f || Character->Health >= Character->MaxHealth)
	{
		bHeal = false;
		AmountToHeal = 0.0f;
		HealingRate = 0.0f;
	}
}
