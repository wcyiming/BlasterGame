// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Public/Weapon/Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget) {
	Super::Fire(HitTarget);
	if (!HasAuthority()) {
		return;
	}
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket) {
		UE_LOG(LogTemp, Warning, TEXT("Fired1"));
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();
		if (ProjectileClass && InstigatorPawn) {
			FActorSpawnParameters SpawnParams;
			SpawnParams.Instigator = InstigatorPawn;
			SpawnParams.Owner = GetOwner();

			UE_LOG(LogTemp, Warning, TEXT("Fired2"));
			UWorld* World = GetWorld();
			if (World) {
				World->SpawnActor<AProjectile>(
					ProjectileClass, 
					SocketTransform.GetLocation(),
					TargetRotation,
					SpawnParams
				);
			}
		}
	}
}
