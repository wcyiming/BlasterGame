// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/PickupSpawnPoint.h"
#include "Pickups/Pickup.h"

// Sets default values
APickupSpawnPoint::APickupSpawnPoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true; // Enable replication for this actor
}

// Called when the game starts or when spawned
void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();
	StartSpawnPickupTimer((AActor*)nullptr); // Start the timer to spawn pickups
}

void APickupSpawnPoint::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

}



void APickupSpawnPoint::SpawnPickup() {
	int32 NumPickupClasses = PickupClasses.Num();
	if (NumPickupClasses <= 0) {
		UE_LOG(LogTemp, Warning, TEXT("No pickup classes defined in %s"), *GetName());
		return;
	}
	int32 RandomIndex = FMath::RandRange(0, NumPickupClasses - 1);
	SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[RandomIndex], GetActorTransform());
	if (HasAuthority() && SpawnedPickup) {
		SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnPickupTimer);
	}
}

void APickupSpawnPoint::SpawnPickupTimerFinished() {
	if (HasAuthority()) {
		SpawnPickup();
	} else {
		// If this is not the server, we should not spawn pickups
		UE_LOG(LogTemp, Warning, TEXT("SpawnPickupTimerFinished called on non-authoritative instance of %s"), *GetName());
	}
}

void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor) {
	const float SpawnTime = FMath::RandRange(SpawnPickupTimeMin, SpawnPickupTimeMax);
	GetWorldTimerManager().SetTimer(SpawnPickupTimer, this, &APickupSpawnPoint::SpawnPickupTimerFinished, SpawnTime, false);
}

// Called every frame

