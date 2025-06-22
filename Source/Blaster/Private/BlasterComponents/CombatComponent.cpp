// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/CombatComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"

#include "Blaster/Public/Weapon/Weapon.h"
#include "Blaster/Public/Character/BlasterCharacter.h"
#include "Blaster/Public/PlayerController/BlasterPlayerController.h"
#include "Blaster/Public/Character/BlasterAnimInstance.h"
#include "Blaster/Public/Weapon/Projectile.h"
#include "Blaster/Public/Weapon/Shotgun.h"

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

		if (FatherCharacter->HasAuthority()) {
			InitializeCarriedAmmo();
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
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
}

void UCombatComponent::SetAiming(bool bIsAiming) {
	if (FatherCharacter == nullptr || EquippedWeapon == nullptr) {
		return;
	}
	bAiming = bIsAiming;
	if (!FatherCharacter->HasAuthority()) {
		ServerSetAiming(bIsAiming);
	}
	if (FatherCharacter) {
		FatherCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
	if (FatherCharacter->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Sniper) {
		FatherCharacter->ShowSniperScopeWidget(bIsAiming);
	}
	if (FatherCharacter->IsLocallyControlled()) bAimingButtonPressed = bIsAiming;
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming) {
	bAiming = bIsAiming;
	if (FatherCharacter) {
		FatherCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}


void UCombatComponent::OnRep_Aiming() {
	if (FatherCharacter && FatherCharacter->IsLocallyControlled()) {
		bAiming = bAimingButtonPressed;
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
	if (EquippedWeapon->IsEmpty()) {
		Reload();
	}
}

bool UCombatComponent::CanFire() {
	if (EquippedWeapon == nullptr || FatherCharacter == nullptr) {
		return false;
	}
	if (bLocallyReloading) {
		return false;
	}
	if (!EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun) return true;
	return !EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::FireButtonPressed(bool bPressed) {
	bFireButtonPressed = bPressed;
	if (bFireButtonPressed) {
		Fire();
	}
}


void UCombatComponent::Fire() {
	if (CanFire()) {

		if (EquippedWeapon) {
			CrosshairShootingFactor += 0.5f;
			CrosshairShootingFactor = FMath::Min(CrosshairShootingFactor, 3.f);

			switch (EquippedWeapon->FireType) {
			case EFireType::EFT_Projectile:
				FireProjectileWeapon();
				break;
			case EFireType::EFT_HitScan:
				FireHitScanWeapon();
				break;
			case EFireType::EFT_Shotgun:
				FireShotgun();
				break;
			}

		}
		StartFireTimer();
	}
}

void UCombatComponent::FireProjectileWeapon() {
	if (EquippedWeapon && FatherCharacter) {
		CrosshairHitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(CrosshairHitTarget) : CrosshairHitTarget;
		if (!FatherCharacter->HasAuthority()) LocalFire(CrosshairHitTarget);
		ServerFire(CrosshairHitTarget, EquippedWeapon->FireDelay);
	}

}

void UCombatComponent::FireHitScanWeapon() {
	if (EquippedWeapon && FatherCharacter) {
		CrosshairHitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(CrosshairHitTarget) : CrosshairHitTarget;
		if (!FatherCharacter->HasAuthority()) LocalFire(CrosshairHitTarget);
		ServerFire(CrosshairHitTarget, EquippedWeapon->FireDelay);
	}
}

void UCombatComponent::FireShotgun() {
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (Shotgun && FatherCharacter) {
		TArray<FVector_NetQuantize> HitTargets;
		Shotgun->ShotunTraceEndWithScatter(CrosshairHitTarget, HitTargets);
		if (!FatherCharacter->HasAuthority()) LocalShotgunFire(HitTargets);
		ServerShotgunFire(HitTargets, EquippedWeapon->FireDelay);
	}

}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget) {
	if (EquippedWeapon == nullptr || FatherCharacter == nullptr) {
		return;
	}

	if (FatherCharacter && CombatState == ECombatState::ECS_Unoccupied) {
		FatherCharacter->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::LocalShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets) {
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (Shotgun == nullptr || FatherCharacter == nullptr) {
		return;
	}
	if (CombatState == ECombatState::ECS_Reloading || CombatState == ECombatState::ECS_Unoccupied) {
		bLocallyReloading = false;
		FatherCharacter->PlayFireMontage(bAiming);
		Shotgun->FireShotgun(TraceHitTargets);
		CombatState = ECombatState::ECS_Unoccupied;

	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, float FireDelay) {
	MulticastFire(TraceHitTarget);
}

bool UCombatComponent::ServerFire_Validate(const FVector_NetQuantize& TraceHitTarget, float FireDelay) {
	if (EquippedWeapon) {
		bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->FireDelay, FireDelay, 0.01f);
		return bNearlyEqual;
	}
	return true;
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget) {
	if (FatherCharacter && FatherCharacter->IsLocallyControlled() && !FatherCharacter->HasAuthority()) {
		return;
	}
	LocalFire(TraceHitTarget);
}

void UCombatComponent::ServerShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay) {
	MulticastShotgunFire(TraceHitTargets);
}

bool UCombatComponent::ServerShotgunFire_Validate(const TArray<FVector_NetQuantize>& TraceHitTarget, float FireDelay) {
	if (EquippedWeapon) {
		bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->FireDelay, FireDelay, 0.01f);
		return bNearlyEqual;
	}
	return true;
}

void UCombatComponent::MulticastShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets) {
	if (FatherCharacter && FatherCharacter->IsLocallyControlled() && !FatherCharacter->HasAuthority()) {
		return;
	}
	LocalShotgunFire(TraceHitTargets);
}

void UCombatComponent::OnRep_CarriedAmmo() {
	FatherController = FatherController == nullptr ? Cast<ABlasterPlayerController>(FatherCharacter->Controller) : FatherController;
	if (FatherController) {
		FatherController->SetHUDCarriedAmmo(CarriedAmmo);
	}
	bool bJumpToShotgunEnd = CombatState == ECombatState::ECS_Reloading && EquippedWeapon != nullptr && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun && CarriedAmmo == 0;
	if (bJumpToShotgunEnd) {
		JumpToShotgunEnd();
	}
}

void UCombatComponent::InitializeCarriedAmmo() {
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARCarriedAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SMG, StartingSmgAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Sniper, StartingSniperAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartingGrenadeLauncherAmmo);
}


void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip) {
	if (FatherCharacter == nullptr || WeaponToEquip == nullptr) {
		return;
	}

	if (CombatState != ECombatState::ECS_Unoccupied) {
		return;
	}

	if (EquippedWeapon != nullptr && SecondaryWeapon == nullptr) {
		EquipSecondaryWeapon(WeaponToEquip);
	} else {
		EquipPrimaryWeapon(WeaponToEquip);
	}



	FatherCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	FatherCharacter->bUseControllerRotationYaw = true;
}

void UCombatComponent::SwapWeapons() {
	if (CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr || SecondaryWeapon == nullptr) {
		return;
	}
	FatherCharacter->PlaySwapMontage();
	CombatState = ECombatState::ECS_SwappingWeapons;

	AWeapon* TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon;



}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip) {
	DropEquippedWeapon();

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetOwner(FatherCharacter);
	EquippedWeapon->SetHUDAmmo();

	UpdateCarriedAmmo();

	PlayEquipWeaponSound(EquippedWeapon);

	if (EquippedWeapon->IsEmpty()) {
		Reload();
	}
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip) {
	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(SecondaryWeapon);
	SecondaryWeapon->SetOwner(FatherCharacter);
	PlayEquipWeaponSound(SecondaryWeapon);
}

void UCombatComponent::PlayEquipWeaponSound(AWeapon* Weapon) {
	if (FatherCharacter&& Weapon && Weapon->EquipSound) {
		UGameplayStatics::PlaySoundAtLocation(
			this,
			Weapon->EquipSound,
			FatherCharacter->GetActorLocation()
		);
	}
}

void UCombatComponent::UpdateCarriedAmmo() {
	if (EquippedWeapon == nullptr || FatherCharacter == nullptr) {
		return;
	}
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) {
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	FatherController = FatherController == nullptr ? Cast<ABlasterPlayerController>(FatherCharacter->Controller) : FatherController;
	if (FatherController) {
		FatherController->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::ShowAttachedGrenade(bool bShowGrenade) {
	if (FatherCharacter && FatherCharacter->GetAttachedGrenade()) {
		FatherCharacter->GetAttachedGrenade()->SetVisibility(bShowGrenade);
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach) {
	if (FatherCharacter == nullptr || FatherCharacter->GetMesh() == nullptr || ActorToAttach == nullptr) {
		return;
	}
	const USkeletalMeshSocket* HandSocket = FatherCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket) {
		HandSocket->AttachActor(ActorToAttach, FatherCharacter->GetMesh());
	}
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach) {
	if (FatherCharacter == nullptr || FatherCharacter->GetMesh() == nullptr || ActorToAttach == nullptr) {
		return;
	}
	const USkeletalMeshSocket* HandSocket = FatherCharacter->GetMesh()->GetSocketByName(FName("LeftHandSocket"));
	if (HandSocket) {
		HandSocket->AttachActor(ActorToAttach, FatherCharacter->GetMesh());
	}
}

void UCombatComponent::AttachActorToBackpack(AActor* ActorToAttach) {
	if (FatherCharacter == nullptr || FatherCharacter->GetMesh() == nullptr || ActorToAttach == nullptr) {
		return;
	}
	const USkeletalMeshSocket* BackpackSocket = FatherCharacter->GetMesh()->GetSocketByName(FName("BackpackSocket"));
	if (BackpackSocket) {
		BackpackSocket->AttachActor(ActorToAttach, FatherCharacter->GetMesh());
	}
}

void UCombatComponent::DropEquippedWeapon() {
	if (EquippedWeapon) {
		EquippedWeapon->Dropped();
	}
}

void UCombatComponent::OnRep_EquippedWeapon() {
	if (EquippedWeapon && FatherCharacter) {
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);
		FatherCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
		FatherCharacter->bUseControllerRotationYaw = true;
		PlayEquipWeaponSound(EquippedWeapon);
		EquippedWeapon->SetHUDAmmo();
	}
}

void UCombatComponent::OnRep_SecondaryWeapon() {
	if (SecondaryWeapon && FatherCharacter) {
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
		AttachActorToBackpack(SecondaryWeapon);
		PlayEquipWeaponSound(SecondaryWeapon);
	}
}

void UCombatComponent::ShotgunShellReload() {

	if (FatherCharacter && FatherCharacter->HasAuthority()) {
		UpdateShotgunAmmoValues();
	}

}

void UCombatComponent::Reload() {
	if (CarriedAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon && !EquippedWeapon->IsFull() && !bLocallyReloading) {
		ServerReload();
		HandleReload();
		bLocallyReloading = true;
	}
}

void UCombatComponent::FinishReloading() {
	if (FatherCharacter == nullptr || EquippedWeapon == nullptr) return;
	bLocallyReloading = false;
	if (FatherCharacter->HasAuthority()) {
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
	if (bFireButtonPressed) {
		Fire();
	}
}

void UCombatComponent::FinishSwap() {
	if (FatherCharacter && FatherCharacter->HasAuthority()) {
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
}

void UCombatComponent::FinishSwapAttachWeapons() {
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	PlayEquipWeaponSound(EquippedWeapon);
	if (EquippedWeapon->IsEmpty()) {
		Reload();
	}

	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(SecondaryWeapon);
}

void UCombatComponent::HandleReload() {
	FatherCharacter->PlayReloadMontage();
}

int32 UCombatComponent::AmountToReload() {
	if (EquippedWeapon == nullptr || FatherCharacter == nullptr) {
		return 0;
	}
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) {
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];	
		int32 Least = FMath::Min(RoomInMag, AmountCarried);
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return 0;
}


void UCombatComponent::ServerReload_Implementation() {
	if (FatherCharacter == nullptr || EquippedWeapon == nullptr) return;

	CombatState = ECombatState::ECS_Reloading;
	if (!FatherCharacter->IsLocallyControlled()) {
		HandleReload();
	}

}

void UCombatComponent::UpdateAmmoValues() {
	if (FatherCharacter == nullptr || EquippedWeapon == nullptr) return;

	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) {
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	FatherController = FatherController == nullptr ? Cast<ABlasterPlayerController>(FatherCharacter->Controller) : FatherController;
	if (FatherController) {
		FatherController->SetHUDCarriedAmmo(CarriedAmmo);
	}
	EquippedWeapon->AddAmmo(ReloadAmount);
}


void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount) {
	if (CarriedAmmoMap.Contains(WeaponType)) {
		CarriedAmmoMap[WeaponType] = FMath::Clamp(
			CarriedAmmoMap[WeaponType] + AmmoAmount,
			0,
			MaxCarriedAmmo
		);
		UpdateCarriedAmmo();
	}
	if (EquippedWeapon && EquippedWeapon->IsEmpty() && EquippedWeapon->GetWeaponType() == WeaponType) {
		Reload();
	}
}

void UCombatComponent::UpdateShotgunAmmoValues() {
	if (FatherCharacter == nullptr || EquippedWeapon == nullptr) return;

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) {
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	FatherController = FatherController == nullptr ? Cast<ABlasterPlayerController>(FatherCharacter->Controller) : FatherController;
	if (FatherController) {
		FatherController->SetHUDCarriedAmmo(CarriedAmmo);
	}
	EquippedWeapon->AddAmmo(1);
	bCanFire = true;
	if (EquippedWeapon->IsFull() || CarriedAmmo == 0) {
		JumpToShotgunEnd();
	}
}

void UCombatComponent::JumpToShotgunEnd() {

	UAnimInstance* AnimInstance = FatherCharacter->GetMesh()->GetAnimInstance();
	if (AnimInstance && FatherCharacter->GetReloadMontage()) {
		AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
	}
}

void UCombatComponent::OnRep_CombatState() {
	switch (CombatState) {
	case ECombatState::ECS_Unoccupied:
		if (bFireButtonPressed) {
			Fire();
		}
		break;
	case ECombatState::ECS_Reloading:
		if (FatherCharacter && !FatherCharacter->IsLocallyControlled()) {
			HandleReload();
		}
		break;
	case ECombatState::ECS_ThrowingGrenade:
		if (FatherCharacter && !FatherCharacter->IsLocallyControlled()) {
			FatherCharacter->PlayThrowGrenadeMontage();
			AttachActorToLeftHand(EquippedWeapon);
			ShowAttachedGrenade(true);
		}
		break;
	case ECombatState::ECS_SwappingWeapons:
		if (FatherCharacter && !FatherCharacter->IsLocallyControlled()) {
			FatherCharacter->PlaySwapMontage();
		}
		break;
	case ECombatState::ECS_MAX:
		break;
	default:
		break;
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

void UCombatComponent::ThrowGrenade() {
	if (CombatState != ECombatState::ECS_Unoccupied || FatherCharacter == nullptr || EquippedWeapon == nullptr) {
		return;
	}
	if (GrenadeCount <= 0) {
		return;
	}

	CombatState = ECombatState::ECS_ThrowingGrenade;

	if (FatherCharacter) {
		FatherCharacter->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);
	}
	if (FatherCharacter && !FatherCharacter->HasAuthority()) {
		ServerThrowGrenade();
	}
	if (FatherCharacter && FatherCharacter->HasAuthority()) {
		GrenadeCount = FMath::Clamp(GrenadeCount - 1, 0, MaxGrenades);
		UpdateHUDGrenades();
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation() {
	if (GrenadeCount <= 0) {
		return;
	}

	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (FatherCharacter) {
		FatherCharacter->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);
	}
	GrenadeCount = FMath::Clamp(GrenadeCount - 1, 0, MaxGrenades);
	UpdateHUDGrenades();
}

void UCombatComponent::ThrowGrenadeFinished() {
	CombatState = ECombatState::ECS_Unoccupied;
	AttachActorToRightHand(EquippedWeapon);
}

void UCombatComponent::LaunchGrenade() {
	ShowAttachedGrenade(false);
	if (FatherCharacter && FatherCharacter->IsLocallyControlled()) {
		ServerLaunchGrenade(CrosshairHitTarget);
	}
}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target) {
	if (FatherCharacter && GrenadeClass && FatherCharacter->GetAttachedGrenade()) {
		const FVector StartingLocation = FatherCharacter->GetAttachedGrenade()->GetComponentLocation();
		FVector ToTarget = CrosshairHitTarget - StartingLocation;
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = FatherCharacter;
		SpawnParams.Instigator = FatherCharacter;
		UWorld* World = GetWorld();
		if (World) {
			World->SpawnActor<AProjectile>(
				GrenadeClass,
				StartingLocation,
				ToTarget.Rotation(),
				SpawnParams
			);
		}
	}
}

void UCombatComponent::OnRep_GrenadeCount() {
	UpdateHUDGrenades();
}

void UCombatComponent::UpdateHUDGrenades() {
	FatherController = FatherController == nullptr ? Cast<ABlasterPlayerController>(FatherCharacter->Controller) : FatherController;
	if (FatherController) {
		FatherController->SetHUDGrenades(GrenadeCount);
	}
}


bool UCombatComponent::ShouldSwapWeapons() {
	return EquippedWeapon != nullptr && SecondaryWeapon != nullptr;
}