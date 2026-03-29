#pragma once

#define LINETRACE_LENGTH 80000.0f

UENUM(Blueprintable)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
	EWT_Pistol UMETA(DisplayName = "Pistol"),
	EWT_SubmachineGun UMETA(DisplayName = "SubMachine Gun"),
	EWT_Shotgun UMETA(DisplayName = "Shotgun"),
	EWT_SniperRifle UMETA(DisplayName = "Sniper Rifle"),
	EWT_GrenadeLauncher UMETA(DisplayName = "Grenade Launcher"),
	EWT_MAX UMETA(DisplayName = "DefaultMax")
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_HitScan UMETA(DisplayName = "HitScan"),
	EFT_Projectile UMETA(DisplayName = "Projectile"),
	EFT_Shotgun UMETA(DisplayName = "Shotgun")
};

UENUM(BlueprintType)
enum class EWeaponOutlineStencil : uint8
{
	EWOS_None = 0 UMETA(Hidden),
	EWOS_Purple = 250 UMETA(DisplayName = "Purple"),
	EWOS_Blue = 251 UMETA(DisplayName = "Blue"),
	EWOS_TAN = 252 UMETA(DisplayName = "Black")
};
