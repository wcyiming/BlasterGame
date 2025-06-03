// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/BlasterPlayerState.h"
#include "Blaster/Public/Character/BlasterCharacter.h"
#include "Blaster/Public/PlayerController/BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerState, Defeats);
}

void ABlasterPlayerState::OnRep_Score() {
	Super::OnRep_Score();

	Character = Character ? Character : Cast<ABlasterCharacter>(GetPawn());
	if (Character) {
		Controller = Controller ? Controller : Cast<ABlasterPlayerController>(Character->GetController());
		if (Controller) {
			Controller->SetHUDScore(GetScore());
		}
	}
}

void ABlasterPlayerState::OnRep_Defeats() {
	Character = Character ? Character : Cast<ABlasterCharacter>(GetPawn());
	if (Character) {
		Controller = Controller ? Controller : Cast<ABlasterPlayerController>(Character->GetController());
		if (Controller) {
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

void ABlasterPlayerState::AddToScore(float ScoreAmount) {
	SetScore(GetScore() + ScoreAmount);
	Character = Character ? Character : Cast<ABlasterCharacter>(GetPawn());
	if (Character) {
		Controller = Controller ? Controller : Cast<ABlasterPlayerController>(Character->GetController());
		if (Controller) {
			Controller->SetHUDScore(GetScore());
		}
	}
}

void ABlasterPlayerState::AddToDefeats(int32 DefeatAmount) {
	Defeats += DefeatAmount;
	Character = Character ? Character : Cast<ABlasterCharacter>(GetPawn());
	if (Character) {
		Controller = Controller ? Controller : Cast<ABlasterPlayerController>(Character->GetController());
		if (Controller) {
			Controller->SetHUDDefeats(Defeats);
		}
	}
}
