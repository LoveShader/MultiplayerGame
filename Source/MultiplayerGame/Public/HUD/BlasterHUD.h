// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

class UAnnouncement;

USTRUCT(BlueprintType)
struct FHUDPackage
{
public:
	GENERATED_BODY()

	class UTexture2D* CrossHairCenter;
	UTexture2D* CrossHairLeft;
	UTexture2D* CrossHairRight;
	UTexture2D* CrossHairTop;
	UTexture2D* CrossHairBottom;
	float CrossHairSpread;
	FColor CrossHairColor;
};
class UCharacterOverlay;

/**
 * 
 */
UCLASS()
class MULTIPLAYERGAME_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
	void AddCharacterOverlay();
	void AddAnnouncement();
protected:
	virtual void BeginPlay() override;
private:
	void DrawCrossHair(UTexture2D* Texture, const FVector2D& ViewportCenter, const FVector2D& Spread, const FColor& Color);
	FHUDPackage HUDPackage;

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f;
	
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> AnnouncementClass;
public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
	/**
	 * Character Overlay Property
	 */
	UCharacterOverlay* CharacterOverlay;

	UAnnouncement* Announcement;
};	

