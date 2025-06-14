// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Sound/SoundCue.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"

#include "Weapon/RocketMovementComponent.h"

AProjectileRocket::AProjectileRocket() {
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
}

void AProjectileRocket::BeginPlay() {
	Super::BeginPlay();

	if (!HasAuthority()) {
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);
	}

	SpawnTrailSystem();

	if (ProjectileLoop && ProjectileLoopAttenuation) {
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.0f,
			1.0f,
			0.0f,
			ProjectileLoopAttenuation,
			(USoundConcurrency*)nullptr,
			false
		);
	}
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) {
	if (OtherActor == GetOwner()) {
		return; // Ignore hits with the owner
	}

	ExplodeDamage();

	StartDestroyTimer();

	if (ImpactParticles) {
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ImpactParticles,
			GetActorTransform()
		);
	}
	if (ImpactSound) {
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ImpactSound,
			GetActorLocation()
		);
	}
	if (ProjectileMesh) {
		ProjectileMesh->SetVisibility(false);
	}
	if (CollisionBox) {
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (TrailComponent && TrailComponent->GetSystemInstance()) {
		TrailComponent->Deactivate();
	}
	if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying()) {
		ProjectileLoopComponent->Stop();
	}
}

void AProjectileRocket::Destroyed() {
}
