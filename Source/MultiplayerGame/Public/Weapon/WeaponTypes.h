#pragma once

UENUM(Blueprintable)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
	EWT_MAX UMETA(DisplayName = "DefaultMax")
};