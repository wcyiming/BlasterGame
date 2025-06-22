// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

namespace MatchState {
	extern BLASTER_API const FName Cooldown; // Match is in cooldown state, no players can join

}

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ABlasterGameMode();
	virtual void Tick(float DeltaTime) override;

	virtual void PlayerElimated(
		class ABlasterCharacter* ElimmedCharacter,
		class ABlasterPlayerController* VictimController,
		class ABlasterPlayerController* AttackerController
	);

	virtual void RequestRespawn(class ABlasterCharacter* ElimmedCharacter, class AController* ElimmedController);
	void PlayerLeftGame(class ABlasterPlayerState* PlayerLeaving);

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f; // Time before match starts

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f; // Time before match starts

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f; // Time before match starts

	float LevelStartingTime = 0.f; // Time when level started, used for match time calculation

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

private:
	float CountdownTime = 0.f; // Time left in match

public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
	
};
