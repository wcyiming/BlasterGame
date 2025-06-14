// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/BuffComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Blaster/Public/Character/BlasterCharacter.h"

// Sets default values for this component's properties
UBuffComponent::UBuffComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UBuffComponent::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed) {
	InitialBaseSpeed = BaseSpeed;
	InitialCrouchSpeed = CrouchSpeed;
}

// Called every frame
void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealRampUp(DeltaTime);
	ShieldRampUp(DeltaTime);
	// ...
}

void UBuffComponent::Heal(float HealAmount, float HealingTime) {
	bHealing = true;
	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
}

void UBuffComponent::ReplenishShield(float ShieldAmount, float ShieldReplenishTime) {
	bShielding = true;
	ShieldReplenishRate = ShieldAmount / ShieldReplenishTime;
	AmountToShield += ShieldAmount;
}

void UBuffComponent::HealRampUp(float DeltaTime) {
	if (!bHealing || FatherCharacter == nullptr || FatherCharacter->IsElimmed() ) {
		return;
	}

	const float HealThisFrame = HealingRate * DeltaTime;
	FatherCharacter->SetHealth(FMath::Clamp(FatherCharacter->GetHealth() + HealThisFrame, 0.f, FatherCharacter->GetMaxHealth()));
	FatherCharacter->UpdateHUDHealth();
	AmountToHeal -= HealThisFrame;
	if (AmountToHeal <= 0.f || FatherCharacter->GetHealth() >= FatherCharacter->GetMaxHealth()) {
		bHealing = false;
		AmountToHeal = 0.f;
	}
}

void UBuffComponent::ShieldRampUp(float DeltaTime) {
	if (!bShielding || FatherCharacter == nullptr || FatherCharacter->IsElimmed()) {
		return;
	}

	const float ReplenishThisFrame = ShieldReplenishRate * DeltaTime;
	FatherCharacter->SetShield(FMath::Clamp(FatherCharacter->GetShield() + ReplenishThisFrame, 0.f, FatherCharacter->GetMaxShield()));
	FatherCharacter->UpdateHUDShield();
	AmountToShield -= ReplenishThisFrame;
	if (AmountToShield <= 0.f || FatherCharacter->GetShield() >= FatherCharacter->GetMaxShield()) {
		bShielding = false;
		AmountToShield = 0.f;
	}
}


void UBuffComponent::BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime) {
	if (FatherCharacter == nullptr || FatherCharacter->IsElimmed()) {
		return;
	}

	FatherCharacter->GetWorldTimerManager().SetTimer(SpeedBuffTimer, this, &UBuffComponent::ResetSpeeds, BuffTime);
	if (FatherCharacter->GetCharacterMovement()) {
		FatherCharacter->GetCharacterMovement()->MaxWalkSpeed = BuffBaseSpeed;
		FatherCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = BuffCrouchSpeed;
	}
	MulticastSpeedBuff(BuffBaseSpeed, BuffCrouchSpeed);
}


void UBuffComponent::ResetSpeeds() {
	if (FatherCharacter == nullptr || FatherCharacter->IsElimmed() || !FatherCharacter->GetCharacterMovement()) {
		return;
	}
	if (FatherCharacter->GetCharacterMovement()) {
		FatherCharacter->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
		FatherCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;
	}
	MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);
}


void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed) {
	if (FatherCharacter && FatherCharacter->GetCharacterMovement()) {
		FatherCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
		FatherCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	}
}

void UBuffComponent::SetInitialJump(float JumpZVelocity) {
	InitialJumpZVelocity = JumpZVelocity;
}


void UBuffComponent::BuffJump(float JumpZVelocityBuff, float JumpBuffTime) {
	if (FatherCharacter == nullptr || FatherCharacter->IsElimmed()) {
		return;
	}
	FatherCharacter->GetWorldTimerManager().SetTimer(JumpBuffTimer, this, &UBuffComponent::ResetJump, JumpBuffTime);
	if (FatherCharacter->GetCharacterMovement()) {
		FatherCharacter->GetCharacterMovement()->JumpZVelocity = JumpZVelocityBuff;
	}
	MulticastJumpBuff(JumpZVelocityBuff);

}

void UBuffComponent::ResetJump() {
	if (FatherCharacter->GetCharacterMovement()) {
		FatherCharacter->GetCharacterMovement()->JumpZVelocity = InitialJumpZVelocity;
	}
	MulticastJumpBuff(InitialJumpZVelocity);
}

void UBuffComponent::MulticastJumpBuff_Implementation(float JumpZVelocityBuff) {
	if (FatherCharacter && FatherCharacter->GetCharacterMovement()) {
		FatherCharacter->GetCharacterMovement()->JumpZVelocity = JumpZVelocityBuff;
	}
}
