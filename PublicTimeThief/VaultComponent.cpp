#include "VaultComponent.h"
#include "DrawDebugHelpers.h"
#include "PlayerStates/PlayerStateBase.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values for this component's properties
UVaultComponent::UVaultComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UVaultComponent::BeginPlay()
{
	Super::BeginPlay();

	// Get Owner Pointer
	Thief = Cast<AThief>(GetOwner());
}


// Called every frame
void UVaultComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UVaultComponent::CharacterCanVault(FVector& OutVaultToVector)
{
	// Setup
	bool bCanVault = false;
	bool bHitOnce = false;

	FVector VaultSpot;

	FVector ForwardVector = Thief->GetActorForwardVector();
	FVector ActorLocation = Thief->GetActorLocation();

	float PlayerPitch = Thief->GetControlRotation().Pitch;
	if (PlayerPitch > 90) PlayerPitch -= 360;

	// Setup Capsule for Colliison Trace
	FQuat Quat = FQuat(Thief->GetActorForwardVector(), 0);
	FCollisionQueryParams Params = FCollisionQueryParams();
	Params.AddIgnoredActor(Thief);
	Params.AddIgnoredActor(Thief->Rifle);

	float CapsuleRadius = 10;
	float CapsuleHalfHeight = 88 - 25;

	// Adjust vault check height if Thief is crouched
	if (Thief->GetCurrentState()->StateID == CROUCH)
	{
		ActorLocation.Z -= 10;
		CapsuleHalfHeight -= 15;
	}

	// Setup HitResult
	FHitResult Hit;
	FVector Start = ActorLocation;
	Start.Z += CapsuleHalfHeight;

	DrawDebugSphere(GetWorld(), Start, 10, 10, FColor::Yellow, false, 3);

	FVector End = Start + ForwardVector * (DistanceFromPlayer - CapsuleRadius);

	// Capsule collision trace
	bool bHit = GetWorld()->SweepSingleByChannel(Hit, Start, End, Quat,
		ECollisionChannel::ECC_Pawn, FCollisionShape::MakeCapsule(CapsuleRadius,
			CapsuleHalfHeight), Params);

	DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 20, 4,
		bHit ? FColor::Red : FColor::Green, false, 3);

	DrawDebugCapsule(GetWorld(), End, CapsuleHalfHeight, CapsuleRadius, Quat, FColor::Blue, false, 3);

	if (bHit)
	{
		// Calculate Starting Height for Scan && Calculate Distance for Scan
		float StartHeight, VaultDistance;
		if (PlayerPitch > 0)
		{
			// if looking up, have a greater start height affected by the Thief's view angle
			StartHeight = 120 + 0.35 * PlayerPitch;
		}
		else
		{
			// if looking down, have a lower StartHeight to avoid unintentional vaulting
			StartHeight = (Thief->GetCurrentState()->StateID != CROUCH ? 120 : 70) + 0.35 * PlayerPitch;
		}

		// Vault Distance dependent on Thief's velocity and view angle
		// Faster they are going the further the vault Distance
		// When looking straight up or straight down, vault distance is reduced
		VaultDistance = Thief->GetVelocity().Size() * 0.001 * 50 + 100;
		VaultDistance -= 50 * FMath::Abs(PlayerPitch * 0.0111);

		// Setup vertical line trace to check for walkable surface
		Start = Thief->GetActorLocation() + Thief->GetActorForwardVector() * VaultDistance;
		Start.Z += StartHeight;

		End = Thief->GetActorLocation() + Thief->GetActorForwardVector() * VaultDistance;
		End.Z -= 10;

		DrawDebugSphere(GetWorld(), Start, 10, 10, FColor::Emerald, false, 3);
		DrawDebugSphere(GetWorld(), End, 10, 10, FColor::Orange, false, 3);

		// Set up No Hit Vector for special case of a thin wall/obstacle
		FVector NoHitVector = Start + Thief->GetActorForwardVector() * 100 + FVector(0, 0, Thief->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

		// Traditional line trace
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_Pawn, Params))
		{
			DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10, 10, FColor::Blue, false, 3);

			if (Thief->GetCharacterMovement()->IsWalkable(Hit))
			{
				// Raise OutVaultToVector Z to factor in Capsule Half Height
				OutVaultToVector = Hit.ImpactPoint;
				OutVaultToVector.Z += Thief->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

				DrawDebugPoint(Thief->GetWorld(), OutVaultToVector, 10, FColor::Orange, false, 5);

				return ProjectVaultPath(Thief->GetWorld(), ActorLocation, OutVaultToVector,
					Thief->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), Thief->GetCapsuleComponent()->GetScaledCapsuleRadius(), Quat, Params);
			}
		}
		// Check if the space on the other side of a thin wall is empty
		else if (!Thief->GetWorld()->SweepSingleByChannel(Hit, NoHitVector, NoHitVector, Quat, ECollisionChannel::ECC_Pawn,
			FCollisionShape::MakeCapsule(Thief->GetCapsuleComponent()->GetScaledCapsuleRadius(), Thief->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), Params))
		{
			// Raise OutVaultToVector Z to factor in Capsule Half Height
			OutVaultToVector = Start;
			OutVaultToVector.Z += Thief->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

			DrawDebugPoint(Thief->GetWorld(), OutVaultToVector, 10, FColor::Blue, false, 5);

			return ProjectVaultPath(Thief->GetWorld(),ActorLocation, OutVaultToVector,
			                        Thief->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), Thief->GetCapsuleComponent()->GetScaledCapsuleRadius(), Quat, Params);
		}
	}

	return false;
}

bool UVaultComponent::ProjectVaultPath(UWorld* World, FVector& CurrentLocation, FVector& VaultEnd, float CapsuleHalfHeight, float CapsuleRadius, FQuat Quaternion, FCollisionQueryParams Params) const
{
	// Setup for Capsule traces
	FHitResult Hit;
	FCollisionShape Capsule = FCollisionShape::MakeCapsule(CapsuleRadius - 15, CapsuleHalfHeight);
	FVector FirstSweepEnd = FVector(CurrentLocation.X, CurrentLocation.Y, VaultEnd.Z + 5);
	bool bHit;

	DrawDebugCapsule(World, CurrentLocation, CapsuleHalfHeight, CapsuleRadius, Quaternion, FColor::Green, false, 10);
	DrawDebugCapsule(World, FirstSweepEnd, CapsuleHalfHeight, CapsuleRadius, Quaternion, FColor::Blue, false, 10);
	DrawDebugCapsule(World, VaultEnd, CapsuleHalfHeight, CapsuleRadius, Quaternion, FColor::Yellow, false, 10);

	// First Capsule trace, straight up
	bHit = World->SweepSingleByChannel(Hit, CurrentLocation, FirstSweepEnd, Quaternion, ECollisionChannel::ECC_Pawn, Capsule, Params);

	if(bHit)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Project Vault Path HIT"));
		DrawDebugCapsule(World, Hit.Location, CapsuleHalfHeight, CapsuleRadius, Quaternion, FColor::Purple, false, 10);
		DrawDebugPoint(World, Hit.ImpactPoint, 10, FColor::Red, false, 10);

		return false;
	}

	// Second Capsule trace, from end of first sweep to the final spot
	bHit = World->SweepSingleByChannel(Hit, FirstSweepEnd, VaultEnd + FVector(0,0,5), Quaternion, ECollisionChannel::ECC_Pawn, Capsule, Params);

	if (bHit)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Project Vault Path HIT"));
		DrawDebugCapsule(World, Hit.Location, CapsuleHalfHeight, CapsuleRadius, Quaternion, FColor::Purple, false, 10);
		DrawDebugPoint(World, Hit.ImpactPoint, 10, FColor::Red, false, 10);

		// If the hit occurs within acceptable error, return true
		if((Hit.Location - VaultEnd).Size() < 20)
		{
			return true;
		}

		return false;
	}

	return true;
}


