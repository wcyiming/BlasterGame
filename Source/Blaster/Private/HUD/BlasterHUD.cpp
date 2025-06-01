// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/BlasterHUD.h"
#include "GameFramework/PlayerController.h"
#include "Blaster/Public/HUD/CharacterOverlay.h"


void ABlasterHUD::BeginPlay() {
	Super::BeginPlay();
	AddCharacterOverlay();
}

void ABlasterHUD::AddCharacterOverlay() {
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayClass) {
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		if (CharacterOverlay) {
			CharacterOverlay->AddToViewport();
		}
	}
}


void ABlasterHUD::DrawHUD() {
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport) {
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter(ViewportSize.X * 0.5f, ViewportSize.Y * 0.5f);
		float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;

		if (HUDPackage.CrosshairCenter) {
			FVector2D Spread(0.f, 0.f);
			DrawCrosshairs(HUDPackage.CrosshairCenter, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
		if (HUDPackage.CrosshairLeft) {
			FVector2D Spread(-SpreadScaled, 0.f);
			DrawCrosshairs(HUDPackage.CrosshairLeft, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
		if (HUDPackage.CrosshairRight) {
			FVector2D Spread(SpreadScaled, 0.f);
			DrawCrosshairs(HUDPackage.CrosshairRight, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
		if (HUDPackage.CrosshairTop) {
			FVector2D Spread(0.f, -SpreadScaled);
			DrawCrosshairs(HUDPackage.CrosshairTop, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
		if (HUDPackage.CrosshairBottom) {
			FVector2D Spread(0.f, SpreadScaled);
			DrawCrosshairs(HUDPackage.CrosshairBottom, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
	}
}


void ABlasterHUD::DrawCrosshairs(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor) {
	const float TextureWidth = Texture->GetSurfaceWidth();
	const float TextureHeight = Texture->GetSurfaceHeight();
	const FVector2D TextureDrawPosition(
		ViewportCenter.X - (TextureWidth * 0.5f) + Spread.X,
		ViewportCenter.Y - (TextureHeight * 0.5f) + Spread.Y
	);

	DrawTexture(
		Texture,
		TextureDrawPosition.X,
		TextureDrawPosition.Y,
		TextureWidth,
		TextureHeight,
		0.f, 0.f, 1.f, 1.f,
		CrosshairColor
	);

}
