#include "BlasterComponents/BuffComponent.h"

#include "Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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
		ShieldRampup(DeltaTime);
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

void UBuffComponent::ReplenishShield(float ShieldAmount, float ReplenishTime)
{
	if (Character == nullptr || ShieldAmount <= 0.0f)
	{
		return;
	}

	if (Character->Shield >= Character->MaxShield)
	{
		bReplenishShield = false;
		ShieldReplenishAmount = 0.0f;
		ShieldReplenishRate = 0.0f;
		return;
	}

	bReplenishShield = true;
	ShieldReplenishAmount += ShieldAmount;
	ShieldReplenishRate = ShieldReplenishAmount / FMath::Max(ReplenishTime, KINDA_SMALL_NUMBER);
}

void UBuffComponent::ShieldRampup(float DeltaTime)
{
	if (!bReplenishShield || Character == nullptr)
	{
		return;
	}

	const float ShieldBeforeReplenish = Character->Shield;
	Character->Shield = FMath::Clamp(Character->Shield + ShieldReplenishRate * DeltaTime, 0.0f, Character->MaxShield);
	const float ShieldThisFrame = Character->Shield - ShieldBeforeReplenish;
	ShieldReplenishAmount -= ShieldThisFrame;

	if (Character->IsLocallyControlled())
	{
		Character->UpdateHUDShield();
	}

	if (ShieldReplenishAmount <= 0.0f || Character->Shield >= Character->MaxShield)
	{
		bReplenishShield = false;
		ShieldReplenishAmount = 0.0f;
		ShieldReplenishRate = 0.0f;
	}
}

void UBuffComponent::BuffSpeed(float BaseSpeed, float CrouchSpeed, float BuffTime)
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr)
	{
		return;
	}

	Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	MulticastSpeedBuff(BaseSpeed, CrouchSpeed);

	Character->GetWorldTimerManager().SetTimer(
		SpeedBuffTimer,
		this,
		&UBuffComponent::ResetSpeeds,
		BuffTime
	);
}

void UBuffComponent::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed)
{
	InitialBaseSpeed = BaseSpeed;
	InitialCrouchSpeed = CrouchSpeed;
}

void UBuffComponent::BuffJump(float JumpVelocity, float BuffTime)
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr)
	{
		return;
	}

	Character->GetCharacterMovement()->JumpZVelocity = JumpVelocity;
	MulticastJumpBuff(JumpVelocity);

	Character->GetWorldTimerManager().SetTimer(
		JumpBuffTimer,
		this,
		&UBuffComponent::ResetJumpVelocity,
		BuffTime
	);
}

void UBuffComponent::SetInitialJumpVelocity(float JumpVelocity)
{
	InitialJumpVelocity = JumpVelocity;
}

void UBuffComponent::ResetSpeeds()
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr)
	{
		return;
	}

	Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;
	MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);
}

void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed)
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr)
	{
		return;
	}

	Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}

void UBuffComponent::ResetJumpVelocity()
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr)
	{
		return;
	}

	Character->GetCharacterMovement()->JumpZVelocity = InitialJumpVelocity;
	MulticastJumpBuff(InitialJumpVelocity);
}

void UBuffComponent::MulticastJumpBuff_Implementation(float JumpVelocity)
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr)
	{
		return;
	}

	Character->GetCharacterMovement()->JumpZVelocity = JumpVelocity;
}
