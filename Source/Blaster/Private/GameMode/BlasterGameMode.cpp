// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/BlasterGameMode.h"
#include "Blaster/Public/Character/BlasterCharacter.h"
#include "Blaster/Public/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Blaster/Public/PlayerState/BlasterPlayerState.h"
#include "GameState/BlasterGameState.h"

namespace MatchState {
	const FName Cooldown = FName("Cooldown"); // Match is in cooldown state, no players can join
}

ABlasterGameMode::ABlasterGameMode() {
	bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay() {
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::OnMatchStateSet() {
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It) {
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*It);
		if (BlasterPlayer) {
			BlasterPlayer->OnMatchStateSet(MatchState);
			if (MatchState == MatchState::WaitingToStart) {
				BlasterPlayer->SetHUDMatchCountdown(WarmupTime);
			} else if (MatchState == MatchState::InProgress) {
				BlasterPlayer->SetHUDMatchCountdown(0.f);
			}
		}
	}
}


void ABlasterGameMode::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart) {
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds();
		if (CountdownTime <= 0.f) {
			StartMatch();
		}
	} else if (MatchState == MatchState::InProgress) {
		CountdownTime = WarmupTime + MatchTime - (GetWorld()->GetTimeSeconds() - LevelStartingTime);
		if (CountdownTime <= 0.f) {
			SetMatchState(MatchState::Cooldown);
		}
	}
	if (MatchState == MatchState::Cooldown) {
		CountdownTime = CooldownTime + WarmupTime + MatchTime - (GetWorld()->GetTimeSeconds() - LevelStartingTime);
		if (CountdownTime <= 0.f) {
			RestartGame();
		}
	}
}

void ABlasterGameMode::PlayerElimated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController) {
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState) {
		AttackerPlayerState->AddToScore(1.f);
		if (BlasterGameState) {
			BlasterGameState->UpdateToScore(AttackerPlayerState);
		}
	}

	if (VictimPlayerState) {
		VictimPlayerState->AddToDefeats(1);
	}

	if (ElimmedCharacter) {
		ElimmedCharacter->Elim();
	}
}

void ABlasterGameMode::RequestRespawn(ABlasterCharacter* ElimmedCharacter, AController* ElimmedController) {
	if (ElimmedCharacter) {
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}

	if (ElimmedController) {
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}
