#include "VaultState.h"

UVaultState::UVaultState()
{
	StateName = TEXT("Vault State");
	StateID = VAULT;
}

void UVaultState::StateTick(float DeltaTime)
{
	Super::StateTick(DeltaTime);

	// the Vault is seperated into two stages, going straight up and then over
	// bVaultingZ is true when the vault is in the straight up section
	// otherwise the vault is in the over (X and Y) stage
	if (Thief->bVaultingZ)
	{
		// Interpolate to point
		Thief->SetActorLocation(FMath::VInterpTo(Thief->GetActorLocation(),
			Thief->VaultTargetZLocation, DeltaTime, VaultZInterpolateConstant * InterpolationMultiplier));

		// Once within certain range, go to next stage
		if (Thief->GetActorLocation().Z > Thief->VaultTargetZLocation.Z - 10)
		{
			Thief->bVaultingZ = false;
		}
	}
	else
	{
		// Interpolate to point
		Thief->SetActorLocation(FMath::VInterpTo(Thief->GetActorLocation(),
			Thief->VaultTargetXYZLocation, DeltaTime, VaultInterpolateConstant * InterpolationMultiplier));

		// Once within range, go back to crouching or walking
		if (FVector::Dist(Thief->GetActorLocation(), Thief->VaultTargetXYZLocation) < 35)
		{
			Thief->bVaulting = false;

			if (Manager->LastWalkOrCrouchState)
				Manager->SwitchNextTickByKey(CROUCH);
			else
				Manager->SwitchNextTickByKey(WALK);
		}
	}

	// Set Camera to Nearest Acceptable Pitch
	float Pitch = Thief->GetControlRotation().Pitch;
	Pitch < 90 ? Pitch : Pitch -= 360;

	// Pitch the camera down if looking up
	if(Pitch > 3)
	{
		Thief->AddControllerPitchInput(0.8);
	}
	// Pitch the camera up if looking down
	else if(Pitch < -30)
	{
		Thief->AddControllerPitchInput(-0.8);
	}

	DrawDebugSphere(Thief->GetWorld(), Thief->VaultTargetXYZLocation, 10, 4, FColor::Magenta, false, 3);
}

void UVaultState::MoveForward(float AxisValue)
{
	// No Implementation, can't move
}

void UVaultState::MoveRight(float AxisValue)
{
	// No Implementation, can't move
}

void UVaultState::LookUp(float AxisValue)
{
	// No Implementation, look is controlled by state tick
}

void UVaultState::LookUpRate(float AxisValue)
{
	// No Implementation, look is controlled by state tick
}

void UVaultState::Dodge()
{
	// No Implementation, can't move
}

void UVaultState::Jump()
{
	// No Implementation, can't move
}

void UVaultState::Crouch()
{
	// No Implementation, can't move
}

void UVaultState::OnStateEnter()
{
	Super::OnStateEnter();

	// Turn on camera lag for less jarring movement
	Thief->TurnOnCameraLag();

	// Speed of vault is directly related to player speed (0.0014 = 1/700)
	// Interpolation mulitiplier can only be greater or equal to 1, will not affect player negatively
	float VelocityRatio = Thief->GetVelocity().Size2D() * 0.0014;
	InterpolationMultiplier = VelocityRatio >= 1 ? VelocityRatio : 1;

	// Set the charcter movement mode to NONE
	Thief->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
}

void UVaultState::OnStateExit()
{
	Super::OnStateExit();

	Thief->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	Thief->GetCharacterMovement()->MaxWalkSpeed = Thief->DefaultMaxWalkSpeed;

	Thief->TurnOffCameraLagInTime(0.0);
}
