// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

#include "Blaster/Public/Character/BlasterCharacter.h"
#include "Blaster/Public/PlayerController/BlasterPlayerController.h"

void AShotgun::Fire(const FVector& HitTarget) {
	AWeapon::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket) {
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		TMap<ABlasterCharacter*, uint32> HitMap;
		for (uint32 i = 0; i < NumberOfPellets; i++) {
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
			if (HitCharacter && InstigatorController && HasAuthority()) {
				if (HitMap.Contains(HitCharacter)) {
					HitMap[HitCharacter]++;
				} else {
					HitMap.Emplace(HitCharacter, 1);
				}
			}
			if (ImpactParticles) {
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ImpactParticles,
					FireHit.ImpactPoint,
					FireHit.ImpactNormal.Rotation()
				);
			}
			if (HitSound) {
				UGameplayStatics::PlaySoundAtLocation(
					this,
					HitSound,
					FireHit.ImpactPoint,
					.5f,
					FMath::FRandRange(-0.5f, 0.5f)
				);
			}
		}


		for (auto HitPair : HitMap) {
			ABlasterCharacter* HitCharacter = HitPair.Key;
			if (HitCharacter && InstigatorController && HasAuthority()) {
				UGameplayStatics::ApplyDamage(
					HitCharacter,
					HitPair.Value * Damage,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
		}

	}
}
