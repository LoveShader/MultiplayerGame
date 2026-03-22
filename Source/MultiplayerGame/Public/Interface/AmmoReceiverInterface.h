// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Weapon/WeaponTypes.h"
#include "AmmoReceiverInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UAmmoReceiverInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MULTIPLAYERGAME_API IAmmoReceiverInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual bool CanReceiveAmmo(EWeaponType WeaponType) const = 0;
	virtual void ReceiveAmmo(EWeaponType WeaponType, int32 AmmoAmount) = 0;	
};
