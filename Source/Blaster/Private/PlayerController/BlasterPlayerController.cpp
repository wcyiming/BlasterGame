// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/BlasterPlayerController.h"
#include "HUD/BlasterHUD.h"
#include "HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Blaster/Public/Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Public/GameMode/BlasterGameMode.h"
#include "Blaster/Public/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/Public/BlasterComponents/CombatComponent.h"
#include "Blaster/Public/GameState/BlasterGameState.h"
#include "Blaster/Public/PlayerState/BlasterPlayerState.h"

void ABlasterPlayerController::BeginPlay() {
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	ServerCheckMatchState();

}


void ABlasterPlayerController::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
	SetHUDTime();

	CheckTimeSync(DeltaTime);
	PollInit();
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABlasterPlayerController, MatchState);
}

void ABlasterPlayerController::PollInit() {
	if (CharacterOverlay == nullptr) {
		if (BlasterHUD && BlasterHUD->CharacterOverlay) {
			CharacterOverlay = BlasterHUD->CharacterOverlay;
			if (CharacterOverlay) {
				if(bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
				if(bInitializeScore) SetHUDScore(HUDScore);
				if(bInitializeDefeats) SetHUDDefeats(HUDDefeats);
				if(bInitializeGrenades) SetHUDGrenades(HUDGrenades);
				if(bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
			}
		}
	}
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation() {
	ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode) {
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);

		//if (BlasterHUD && MatchState == MatchState::WaitingToStart) {
		//	BlasterHUD->AddAnnouncement();
		//}
	}
}

void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime) {
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);
	if (BlasterHUD && MatchState == MatchState::WaitingToStart) {
		BlasterHUD->AddAnnouncement();
	}
}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime) {
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime >= TimeSyncFrequency) {
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ABlasterPlayerController::SetHUDTime() {
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) {
		TimeLeft = WarmupTime - (GetServerTime() - LevelStartingTime);
	} else if (MatchState == MatchState::InProgress) {
			TimeLeft = WarmupTime + MatchTime - (GetServerTime() - LevelStartingTime);
	} else if (MatchState == MatchState::Cooldown) {
		TimeLeft = CooldownTime + WarmupTime + MatchTime - (GetServerTime() - LevelStartingTime);
	}

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
	if (HasAuthority()) {
		BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
		if (BlasterGameMode) {
			SecondsLeft = FMath::CeilToInt(BlasterGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}

	if (CountdownInt != SecondsLeft) {
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown) {
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress) {
			SetHUDMatchCountdown(TimeLeft);
		}
	}
	CountdownInt = SecondsLeft;
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth) {
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD && BlasterHUD->CharacterOverlay) {
		if (BlasterHUD->CharacterOverlay->HealthBar) {
			const float HealthPercent = Health / MaxHealth;
			BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		}
		if (BlasterHUD->CharacterOverlay->HealthText) {
			FString HealthString = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
			BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthString));
		}
	} else {
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasterPlayerController::SetHUDShield(float Shield, float MaxShield) {
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD && BlasterHUD->CharacterOverlay) {
		if (BlasterHUD->CharacterOverlay->ShieldBar) {
			const float ShieldPercent = Shield / MaxShield;
			BlasterHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
		}
		if (BlasterHUD->CharacterOverlay->ShieldText) {
			FString ShieldString = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
			BlasterHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldString));
		}
	} else {
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
	}
}

void ABlasterPlayerController::SetHUDScore(float Score) {
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD && BlasterHUD->CharacterOverlay) {
		if (BlasterHUD->CharacterOverlay->ScoreAmount) {
			FString ScoreString = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
			BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreString));
		}
	}
	else {
		bInitializeDefeats = true;
		HUDScore = Score;
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats) {
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD && BlasterHUD->CharacterOverlay) {
		if (BlasterHUD->CharacterOverlay->DefeatsAmount) {
			FString DefeatsString = FString::Printf(TEXT("%d"), Defeats);
			BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsString));
		}
	}
	else {
		bInitializeScore = true;
		HUDDefeats = Defeats;
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo) {
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD && BlasterHUD->CharacterOverlay) {
		if (BlasterHUD->CharacterOverlay->WeaponAmmoAmount) {
			FString AmmoString = FString::Printf(TEXT("%d"), Ammo);
			BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoString));
		}
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo) {
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD && BlasterHUD->CharacterOverlay) {
		if (BlasterHUD->CharacterOverlay->CarriedAmmoAmount) {
			FString AmmoString = FString::Printf(TEXT("%d"), Ammo);
			BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoString));
		}
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime) {
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD && BlasterHUD->CharacterOverlay) {
		if (BlasterHUD->CharacterOverlay->MatchCountdown) {

			if (CountdownTime < 0.f) {
				BlasterHUD->CharacterOverlay->MatchCountdown->SetText(FText());
				return;
			}

			int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
			int32 Seconds = FMath::FloorToInt(CountdownTime) % 60;

			FString CountdownString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
			BlasterHUD->CharacterOverlay->MatchCountdown->SetText(FText::FromString(CountdownString));
		}
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime) {
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD && BlasterHUD->Announcement) {
		if (BlasterHUD->Announcement->WarmupTime) {

			if (CountdownTime < 0.f) {
				BlasterHUD->Announcement->WarmupTime->SetText(FText());
				return;
			}

			int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
			int32 Seconds = FMath::FloorToInt(CountdownTime) % 60;

			FString CountdownString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
			BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownString));
		}
	}
}

void ABlasterPlayerController::SetHUDGrenades(int32 Grenades) {
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD && BlasterHUD->CharacterOverlay) {
		if (BlasterHUD->CharacterOverlay->GrenadesText) {
			FString GrenadeString = FString::Printf(TEXT("%d"), Grenades);
			BlasterHUD->CharacterOverlay->GrenadesText->SetText(FText::FromString(GrenadeString));
		}
	} else {
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void ABlasterPlayerController::OnPossess(APawn* InPawn) {
	Super::OnPossess(InPawn);
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	if (BlasterCharacter) {
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
	}
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest) {
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest) {
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (RoundTripTime * 0.5f);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ABlasterPlayerController::GetServerTime() {
	if (HasAuthority()) {
		return GetWorld()->GetTimeSeconds();
	}
	return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::ReceivedPlayer() {
	Super::ReceivedPlayer();
	// Sync with server clock time as soon as the player is received
	if (IsLocalController()) {
		float ClientRequestTime = GetWorld()->GetTimeSeconds();
		ServerRequestServerTime(ClientRequestTime);
	}
}

void ABlasterPlayerController::OnMatchStateSet(FName State) {
	MatchState = State;

	if (MatchState == MatchState::InProgress) {
		BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
		HandleMatchHasStarted();
	} 
	if (MatchState == MatchState::Cooldown) {
		HandleCooldown();
	}
}

void ABlasterPlayerController::OnRep_MatchState() {
	if (MatchState == MatchState::InProgress) {
		HandleMatchHasStarted();
	}
	if (MatchState == MatchState::Cooldown) {
		HandleCooldown();
	}
}

void ABlasterPlayerController::HandleMatchHasStarted() {
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD) {
		BlasterHUD->AddCharacterOverlay();
		if (BlasterHUD->Announcement) {
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ABlasterPlayerController::HandleCooldown() {
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD) {
		BlasterHUD->CharacterOverlay->RemoveFromParent();
		if (BlasterHUD->Announcement && BlasterHUD->Announcement->AnnouncementText && BlasterHUD->Announcement->InfoText) {
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText = FString::Printf(TEXT("New Match Starts In:"));
			BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			FString InfoText = FString::Printf(TEXT(""));
			if (BlasterGameState) {
				TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;

				if (TopPlayers.Num() == 0) {
					InfoText = FString::Printf(TEXT("No winner."));
				} else if (TopPlayers.Num() == 1) {
					InfoText = FString::Printf(TEXT("Winner: %s"), *TopPlayers[0]->GetPlayerName());
				} else {
					InfoText = FString::Printf(TEXT("Winners: "));
					for (int32 i = 0; i < TopPlayers.Num(); ++i) {
						if (i > 0) {
							InfoText += TEXT(", ");
						}
						InfoText += TopPlayers[i]->GetPlayerName();
					}
				}
			}
			BlasterHUD->Announcement->InfoText->SetText(FText::FromString(InfoText));
		}
	}

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter && BlasterCharacter->GetCombat()) {
		BlasterCharacter->bDisableGameplay = true; // Disable gameplay during cooldown
		BlasterCharacter->GetCombat()->FireButtonPressed(false); // Stop firing
	}
}
