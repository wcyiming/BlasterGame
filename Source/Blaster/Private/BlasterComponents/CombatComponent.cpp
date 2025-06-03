// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/CombatComponent.h"
#include "Blaster/Public/Weapon/Weapon.h"
#include "Blaster/Public/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Blaster/Public/PlayerController/BlasterPlayerController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"

#define TRACE_LENGTH 80000.f

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
}


// Called when the game starts
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (FatherCharacter) {
		FatherCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (FatherCharacter->GetFollowCamera()) {
			DefaultFOV = FatherCharacter->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
	}
}

// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (FatherCharacter && FatherCharacter->IsLocallyControlled()) {
		FHitResult TraceHitResult;
		TraceUnderCrosshairs(TraceHitResult);
		CrosshairHitTarget = TraceHitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::SetAiming(bool bIsAiming) {
	bAiming = bIsAiming;
	if (!FatherCharacter->HasAuthority()) {
		ServerSetAiming(bIsAiming);
	}
	if (FatherCharacter) {
		FatherCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming) {
	bAiming = bIsAiming;
	if (FatherCharacter) {
		FatherCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::InterpFOV(float DeltaTime) {
	if (EquippedWeapon == nullptr || FatherCharacter == nullptr) {
		return;
	}

	if (bAiming) {
		CurrentFOV = FMath::FInterpTo(
			CurrentFOV,
			EquippedWeapon->GetZoomedFOV(),
			DeltaTime,
			EquippedWeapon->GetZoomInterpSpeed()
		);
	} else {
		CurrentFOV = FMath::FInterpTo(
			CurrentFOV,
			DefaultFOV,
			DeltaTime,
			ZoomInterpSpeed
		);
	}

	if (FatherCharacter && FatherCharacter->GetFollowCamera()) {
		FatherCharacter->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::StartFireTimer() {
	if (EquippedWeapon == nullptr || FatherCharacter == nullptr) {
		return;
	}

	bCanFire = false;
	FatherCharacter->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->FireDelay
	);
}

void UCombatComponent::FireTimerFinished() {
	bCanFire = true;
	if (EquippedWeapon == nullptr) {
		return;
	}
	if (bFireButtonPressed && EquippedWeapon->bAutomatic) {
		Fire();
	}
}

bool UCombatComponent::CanFire() {
	if (EquippedWeapon == nullptr || FatherCharacter == nullptr) {
		return false;
	}
	return !EquippedWeapon->IsEmpty() && bCanFire;
}

void UCombatComponent::FireButtonPressed(bool bPressed) {
	bFireButtonPressed = bPressed;
	if (bFireButtonPressed) {
		Fire();
	}
}

void UCombatComponent::Fire() {
	if (CanFire()) {
		ServerFire(CrosshairHitTarget);
		if (EquippedWeapon) {
			CrosshairShootingFactor += 0.5f;
			CrosshairShootingFactor = FMath::Min(CrosshairShootingFactor, 3.f);
		}
		StartFireTimer();
	}
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget) {
	if (EquippedWeapon == nullptr || FatherCharacter == nullptr) {
		return;
	}
	if (FatherCharacter) {
		FatherCharacter->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget) {
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip) {
	if (FatherCharacter == nullptr || WeaponToEquip == nullptr) {
		return;
	}

	if (EquippedWeapon) {
		EquippedWeapon->Dropped();
	}

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = FatherCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket) {
		HandSocket->AttachActor(EquippedWeapon, FatherCharacter->GetMesh());
	}
	EquippedWeapon->SetOwner(FatherCharacter);
	EquippedWeapon->SetHUDAmmo();
	FatherCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	FatherCharacter->bUseControllerRotationYaw = true;
}

void UCombatComponent::OnRep_EquippedWeapon() {
	if (EquippedWeapon && FatherCharacter) {
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* HandSocket = FatherCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket) {
			HandSocket->AttachActor(EquippedWeapon, FatherCharacter->GetMesh());
		}
		FatherCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
		FatherCharacter->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult) {
	FVector2d ViewportSize;
	if (GEngine && GEngine->GameViewport) {
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2d CorsHairLocation = FVector2d(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CorsHairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld) {

		FVector Start = CrosshairWorldPosition;
		if (FatherCharacter) {
			float DistanceToCharacter = (FatherCharacter->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
		}

		FVector End = CrosshairWorldPosition + (CrosshairWorldDirection * TRACE_LENGTH);
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(FatherCharacter);
		CollisionParams.bTraceComplex = true;
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECC_Visibility,
			CollisionParams
		);

		if (!TraceHitResult.bBlockingHit) {
			TraceHitResult.ImpactPoint = End;
			TraceHitResult.ImpactNormal = -CrosshairWorldDirection;
			TraceHitResult.Location = End;
			TraceHitResult.Normal = -CrosshairWorldDirection;
		}

		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>()) {
			HUDPackage.CrosshairColor = FLinearColor::Red;
		} else {
			HUDPackage.CrosshairColor = FLinearColor::White;
		}
	}

}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime) {
	if (FatherCharacter == nullptr || FatherCharacter->Controller == nullptr) {
		return;
	}

	FatherController = FatherController == nullptr ? Cast<ABlasterPlayerController>(FatherCharacter->Controller) : FatherController;
	if (FatherController) {
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(FatherController->GetHUD()) : HUD;
		if (HUD) {

			if (EquippedWeapon) {
				HUDPackage.CrosshairCenter = EquippedWeapon->CrosshairCenter;
				HUDPackage.CrosshairLeft = EquippedWeapon->CrosshairLeft;
				HUDPackage.CrosshairRight = EquippedWeapon->CrosshairRight;
				HUDPackage.CrosshairTop = EquippedWeapon->CrosshairTop;
				HUDPackage.CrosshairBottom = EquippedWeapon->CrosshairBottom;
			} else {
				HUDPackage.CrosshairCenter = nullptr;
				HUDPackage.CrosshairLeft = nullptr;
				HUDPackage.CrosshairRight = nullptr;
				HUDPackage.CrosshairTop = nullptr;
				HUDPackage.CrosshairBottom = nullptr;
			}

			FVector2D WalkSpeedRange(0.f, FatherCharacter->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = FatherCharacter->GetVelocity();
			Velocity.Z = 0.f;

			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(
				WalkSpeedRange,
				VelocityMultiplierRange,
				Velocity.Size()
			);

			if (FatherCharacter->GetCharacterMovement()->IsFalling()) {
				CrosshairInAirFactor = FMath::FInterpTo(
					CrosshairInAirFactor,
					2.25f,
					DeltaTime,
					2.25f
				);
			} else {
				CrosshairInAirFactor = FMath::FInterpTo(
					CrosshairInAirFactor,
					0.f,
					DeltaTime,
					30.f
				);
			}

			if (bAiming) {
				CrosshairAimFactor = FMath::FInterpTo(
					CrosshairAimFactor,
					0.5f,
					DeltaTime,
					30.f
				);
			} else {
				CrosshairAimFactor = FMath::FInterpTo(
					CrosshairAimFactor,
					0.f,
					DeltaTime,
					30.f
				);
			}

			CrosshairShootingFactor = FMath::FInterpTo(
				CrosshairShootingFactor,
				0.f,
				DeltaTime,
				10.f
			);

			// - CrosshairAimFactor for aiming make crosshair smaller
			HUDPackage.CrosshairSpread = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor + CrosshairShootingFactor;

			HUD->SetHUDPackage(HUDPackage);

		}
	}
}