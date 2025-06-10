// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileRocket.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileRocket();
	virtual void Destroyed() override;
	
protected:

	virtual void OnHit(UPrimitiveComponent* HitComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	) override;
	virtual void BeginPlay() override;
	void DestroyTimerFinished();

	UPROPERTY(EditAnywhere, Category = "Rocket")
	class UNiagaraSystem* RocketTrailSystem;

	UPROPERTY()
	class UNiagaraComponent* RocketTrailComponent;

	UPROPERTY(EditAnywhere, Category = "Rocket")
	USoundCue* ProjectileLoop;

	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;

	UPROPERTY(EditAnywhere, Category = "Rocket")
	USoundAttenuation* ProjectileLoopAttenuation;

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RocketMesh;

	FTimerHandle DestroyTimerHandle;

	UPROPERTY(EditAnywhere, Category = "Rocket")
	float DestroyTime = 3.f; // Time before the rocket is destroyed


};
