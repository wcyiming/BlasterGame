// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"
#include "Blaster/Public/Weapon/WeaponTypes.h"


#include "AmmoPickup.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AAmmoPickup : public APickup
{
	GENERATED_BODY()
	
protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

private:
	UPROPERTY(EditAnywhere)
	int32 AmmoAmount = 30; // Amount of ammo to give when picked up

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType = EWeaponType::EWT_AssaultRifle; // Type of weapon this ammo is for
};
