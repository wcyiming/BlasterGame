// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/HealthPickup.h"


#include "Blaster/Public/Character/BlasterCharacter.h"
#include "Blaster/Public/BlasterComponents/BuffComponent.h"

AHealthPickup::AHealthPickup() {
	bReplicates = true;

}

void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter) {
		UBuffComponent* BuffComponent = BlasterCharacter->GetBuff();
		if (BuffComponent) {
			// Heal the character
			BuffComponent->Heal(HealAmount, HealingTime);
		}
	}
	Destroy();
}

