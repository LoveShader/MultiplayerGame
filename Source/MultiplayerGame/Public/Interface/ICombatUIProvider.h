// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ICombatUIProvider.generated.h"

class UCombatComponent;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UICombatUIProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MULTIPLAYERGAME_API IICombatUIProvider
{
	GENERATED_BODY()
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual UCombatComponent* GetCombatComponentForUI() = 0;
};
