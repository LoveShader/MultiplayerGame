// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/BlasterHUD.h"

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (GEngine)
	{
		 GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	const FVector2D ViewportCenter(ViewportSize.X / 2, ViewportSize.Y / 2);

	//HUDPackage.CrossHairSpread need calculate from CombatComponent
	float SpreadScaled = HUDPackage.CrossHairSpread * CrosshairSpreadMax;

	if (HUDPackage.CrossHairCenter)
	{
		FVector2D Spread(0, 0);
		DrawCrossHair(HUDPackage.CrossHairCenter, ViewportCenter, Spread, HUDPackage.CrossHairColor);
	}

	if (HUDPackage.CrossHairLeft)
	{
		FVector2D Spread(-SpreadScaled, 0);
		DrawCrossHair(HUDPackage.CrossHairLeft, ViewportCenter, Spread, HUDPackage.CrossHairColor);
	}

	if (HUDPackage.CrossHairRight)
	{
		FVector2D Spread(SpreadScaled, 0);
		DrawCrossHair(HUDPackage.CrossHairRight, ViewportCenter, Spread, HUDPackage.CrossHairColor);
	}

	if (HUDPackage.CrossHairTop)
	{
		FVector2D Spread(0, -SpreadScaled);
		DrawCrossHair(HUDPackage.CrossHairTop, ViewportCenter, Spread, HUDPackage.CrossHairColor);
	}

	if (HUDPackage.CrossHairBottom)
	{
		FVector2D Spread(0, SpreadScaled);
		DrawCrossHair(HUDPackage.CrossHairBottom, ViewportCenter, Spread, HUDPackage.CrossHairColor);
	}
}

void ABlasterHUD::DrawCrossHair(UTexture2D* Texture, const FVector2D& ViewportCenter, const FVector2D& Spread, const FColor& Color)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();

	const FVector2D DrawPosition(
		ViewportCenter.X - TextureWidth / 2 + Spread.X,
		ViewportCenter.Y - TextureHeight / 2 + Spread.Y
	);

	DrawTexture(
		Texture,
		DrawPosition.X,
		DrawPosition.Y,
		TextureWidth,
		TextureHeight,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		Color);
}

