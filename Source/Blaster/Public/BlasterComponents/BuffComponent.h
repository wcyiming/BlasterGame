// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBuffComponent();
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	friend class ABlasterCharacter;
	void Heal(float HealAmount, float HealingTime);
	void ReplenishShield(float ShieldAmount, float ShieldReplenishTime);
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime);
	void BuffJump(float JumpZVelocityBuff, float JumpBuffTime);
	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed);
	void SetInitialJump(float JumpZVelocity);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	void HealRampUp(float DeltaTime);
	void ShieldRampUp(float DeltaTime);

private:
	UPROPERTY()
	class ABlasterCharacter* FatherCharacter;

	// heal buff
		
	bool bHealing = false;
	float HealingRate = 0;
	float AmountToHeal = 0.f;

	// Shield buff
	bool bShielding = false;
	float ShieldReplenishRate = 0;
	float AmountToShield = 0.f;
	 

	//speed buff

	FTimerHandle SpeedBuffTimer;
	void ResetSpeeds();
	float InitialBaseSpeed = 0.f;
	float InitialCrouchSpeed = 0.f;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	// Jump buff
	FTimerHandle JumpBuffTimer;
	void ResetJump();
	float InitialJumpZVelocity = 0.f;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpZVelocityBuff);

};
