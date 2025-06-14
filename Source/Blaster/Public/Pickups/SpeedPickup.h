// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"
#include "SpeedPickup.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ASpeedPickup : public APickup
{
	GENERATED_BODY()

protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

private:
	
	UPROPERTY(EditAnywhere)
	float BaseSpeedBuff = 1600.f; // Base speed buff value, can be adjusted

	UPROPERTY(EditAnywhere)
	float CrouchSpeedBuff = 800.f; // Crouch speed buff value, can be adjusted

	UPROPERTY(EditAnywhere)
	float SpeedBuffTime = 30.f; // Duration of the speed buff in seconds
	
};
