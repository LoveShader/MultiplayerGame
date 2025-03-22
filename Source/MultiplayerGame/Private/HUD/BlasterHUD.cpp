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

	if (HUDPackage.CrossHairCenter)
	{
		DrawCrossHair(HUDPackage.CrossHairCenter, ViewportCenter);
	}

	if (HUDPackage.CrossHairLeft)
	{
		DrawCrossHair(HUDPackage.CrossHairLeft, ViewportCenter);
	}

	if (HUDPackage.CrossHairRight)
	{
		DrawCrossHair(HUDPackage.CrossHairRight, ViewportCenter);
	}

	if (HUDPackage.CrossHairTop)
	{
		DrawCrossHair(HUDPackage.CrossHairTop, ViewportCenter);
	}

	if (HUDPackage.CrossHairBottom)
	{
		DrawCrossHair(HUDPackage.CrossHairBottom, ViewportCenter);
	}
}

void ABlasterHUD::DrawCrossHair(UTexture2D* Texture, const FVector2D& ViewportCenter)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();

	const FVector2D DrawPosition(
		ViewportCenter.X - TextureWidth / 2,
		ViewportCenter.Y - TextureHeight / 2
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
		FLinearColor::Red);
}

