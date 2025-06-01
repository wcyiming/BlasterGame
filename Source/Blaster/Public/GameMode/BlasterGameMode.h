// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void PlayerElimated(
		class ABlasterCharacter* ElimmedCharacter,
		class ABlasterPlayerController* VictimController,
		class ABlasterPlayerController* AttackerController
	);

	virtual void RequestRespawn(class ABlasterCharacter* ElimmedCharacter, class AController* ElimmedController);
	
};
