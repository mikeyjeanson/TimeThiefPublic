#pragma once

#include "PlayerStateBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/ForceFeedbackAttenuation.h"

UPlayerStateBase::UPlayerStateBase()
{
	StateName = TEXT("Base State");
	StateID = BASE;
}

void UPlayerStateBase::StateTick(float DeltaTime)
{
	// Update FramesSinceGrounded for JumpAssist
	if(Thief->GetCharacterMovement()->IsMovingOnGround())
	{
		FramesSinceGrounded = 0;
	}
	else
	{
		FramesSinceGrounded++;
	}

	if(Manager->bControllerSprintOn)
	{
		Manager->TimeSinceSprintToggle += DeltaTime;

		// If Controller Sprint is toggled, make sure the speed of the player is always above 400 cm/s
		// Have a brief period after Controller Sprint is toggled on where the velocity check is not performed
		if(Manager->TimeSinceSprintToggle > Manager->SprintToggleNoCheckTime && Thief->GetVelocity().Size2D() < 400)
		{
			Manager->bControllerSprintOn = false;
			StopSprint();

			GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Purple, "Toggle Sprint OFF");
		}
	}
}

void UPlayerStateBase::MoveForward(float AxisValue)
{
	if (Thief) 
	{
		// Move slower when aiming
		Thief->MoveForwardAxisValue = AxisValue;
		Thief->AddMovementInput(Thief->GetActorForwardVector() * (Thief->bAimingC ? AxisValue * 0.5 : AxisValue));
	}
}

void UPlayerStateBase::LookUp(float AxisValue)
{	if(Thief)
		Thief->AddControllerPitchInput(AxisValue);
}

void UPlayerStateBase::MoveRight(float AxisValue)
{
	if (Thief)
	{
		// Move slower when aiming
		Thief->MoveRightAxisValue = AxisValue;
		Thief->AddMovementInput(Thief->GetActorRightVector() * (Thief->bAimingC ? AxisValue * 0.5 : AxisValue));
	}
}

void UPlayerStateBase::LookRight(float AxisValue)
{
	if (Thief)
		Thief->AddControllerYawInput(AxisValue);
}

void UPlayerStateBase::LookUpRate(float AxisValue)
{
	if (Thief)
	{
		// Look around slower when aiming
		Thief->AddControllerPitchInput(Thief->RotationRate * Thief->GetWorld()->GetDeltaSeconds() * (Thief->bAimingC ? AxisValue * 0.3 : AxisValue));
	}
}

void UPlayerStateBase::LookRightRate(float AxisValue)
{
	if (Thief)
	{
		// Look around slower when aiming
		Thief->AddControllerYawInput(Thief->RotationRate * Thief->GetWorld()->GetDeltaSeconds() * (Thief->bAimingC ? AxisValue * 0.3 : AxisValue));
	}
}

void UPlayerStateBase::Dodge()
{
	if (Thief)
	{
		// Get Dodge Direction Based on Input Direction
		FVector DodgeDirection = (Thief->GetActorForwardVector() * Thief->MoveForwardAxisValue +
			Thief->GetActorRightVector() * Thief->MoveRightAxisValue).GetSafeNormal();
		
		// Zero Out Z Direction
		FVector Impulse = FVector(DodgeDirection.X, DodgeDirection.Y, 0);

		// Setup Capsule Trace
		FHitResult Hit;
		FVector Start = Thief->GetActorLocation() + DodgeDirection * 150;
		Start.Z -= 10;
		FVector End = Start + DodgeDirection * 200;
		FQuat Quat = FQuat(Thief->GetActorForwardVector(), 0);

		FCollisionQueryParams Params = FCollisionQueryParams();
		Params.AddIgnoredActor(Thief);

		float CapsuleRadius = Thief->GetCapsuleComponent()->GetScaledCapsuleRadius();
		float CapsuleHalfHeight = Thief->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		// Check if dodging off of a ledge, true if not dodging off of a ledge
		bool bHit = Thief->GetWorld()->SweepSingleByChannel(Hit, Start, End, Quat,
			ECollisionChannel::ECC_Pawn, FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight), Params);

		// if dodging on the ground, dodge at regular impulse
		// otherwise dodge at the lesser impulse
		if (Thief->GetCharacterMovement()->IsMovingOnGround() && bHit)
		{
			Thief->LaunchCharacter(Impulse * DodgeForceMultipier, false, false);
			// DrawDebugSphere(Thief->GetWorld(), Hit.ImpactPoint, 20, 4, FColor::Red, false, 3);

			UGameplayStatics::PlaySound2D(Thief->GetWorld(), Thief->DodgeSoundGround, Thief->PlayerSoundClass->Properties.Volume, RAND_PITCH(0.95f, 1.05f));
		}
		else
		{
			Thief->LaunchCharacter(Impulse * DodgeForceMultipier * 0.5, true, false);
			// DrawDebugCapsule(Thief->GetWorld(), End, CapsuleHalfHeight, CapsuleRadius, Quat, FColor::Cyan, false, 3);

			UGameplayStatics::PlaySound2D(Thief->GetWorld(), Thief->DodgeSoundAir, Thief->PlayerSoundClass->Properties.Volume, RAND_PITCH(0.95f, 1.05f));
		}
	}
}

void UPlayerStateBase::Shoot()
{
	if (Thief) 
	{
		// If Using Katana
		if(Thief->bUsingKatana && Thief->bReadyToAttack)
		{
			// If Ready For Left Attack
			if (Thief->bKatanaLeftAttackReady && Thief->LightLeftAttackMontage)
			{
				Thief->ThiefAnimInst->Montage_Play(Thief->LightLeftAttackMontage);
				Thief->bReadyToAttack = Thief->bKatanaLeftAttackReady = false;

				// Start Timer for ready attack
				Thief->GetWorld()->GetTimerManager().SetTimer(Thief->KatanaLeftAttackTimerHandle,
					Thief, &AThief::SetKatanaLeftAttackReady, 0.5f);
				Thief->GetWorld()->GetTimerManager().SetTimer(Thief->KatanaRightAttackTimerHandle,
					Thief, &AThief::SetKatanaRightAttackReady, 0.2f);

				// Enable Katana Hitbox and Start Disable Timer
				Thief->Katana->SetAttacking(false);
				Thief->GetWorld()->GetTimerManager().SetTimer(Thief->Katana->DisableHitboxTimerHandle, Thief->Katana,
					&AThiefKatana::DisableHitbox, 0.35f);

				Manager->bShooting = false;

				int32 SoundIndex = FMath::RandRange(0, Thief->KatanaSoundArray.Num() - 1);
				UGameplayStatics::PlaySound2D(Thief->GetWorld(), Thief->KatanaSoundArray[SoundIndex], Thief->PlayerSoundClass->Properties.Volume, RAND_PITCH(0.95f, 1.05f));
			}
			// Else, Use Right Attack
			else if (Thief->bKatanaRightAttackReady && Thief->LightRightAttackMontage)
			{
				Thief->ThiefAnimInst->Montage_Play(Thief->LightRightAttackMontage);
				Thief->bReadyToAttack = Thief->bKatanaRightAttackReady = false;

				// Start Timer for ready attack
				Thief->GetWorld()->GetTimerManager().SetTimer(Thief->KatanaLeftAttackTimerHandle, Thief, &AThief::SetKatanaLeftAttackReady, 0.3f);

				// Enable Katana Hitbox and Start Disable Timer
				Thief->Katana->SetAttacking(false);
				Thief->GetWorld()->GetTimerManager().SetTimer(Thief->Katana->DisableHitboxTimerHandle, Thief->Katana,
					&AThiefKatana::DisableHitbox, 0.35f);

				Manager->bShooting = false;

				uint8 SoundIndex = FMath::RandRange(0, Thief->KatanaSoundArray.Num() - 1);
				UGameplayStatics::PlaySound2D(Thief->GetWorld(), Thief->KatanaSoundArray[SoundIndex], Thief->PlayerSoundClass->Properties.Volume, RAND_PITCH(0.95f, 1.05f));
			}
		}
		// If using Rifle and Fire returns true
		else if(Thief->bReadyToSwitch && !Thief->bUsingKatana && Thief->Rifle->Fire())
		{
			FVector Location;
			FRotator Rotation;
			Thief->GetController()->GetPlayerViewPoint(Location, Rotation);

			FVector Endpoint = Location + Rotation.Vector() * MaxRange;

			FHitResult Hit;
			FCollisionQueryParams Params;

			Params.AddIgnoredActor(Thief);
			Params.AddIgnoredActor(Thief->Rifle);

			bool bHit = Thief->GetWorld()->LineTraceSingleByChannel(Hit, Location, Endpoint,
				ECollisionChannel::ECC_GameTraceChannel1, Params);

			if (bHit)
			{
				DrawDebugPoint(Thief->GetWorld(), Hit.ImpactPoint, 20, FColor::MakeRandomColor(), false, 2);

				if(AActor* HitActor = Hit.GetActor(); IsValid(HitActor))
				{
					FPointDamageEvent DamageEvent(34.f, Hit, -Rotation.Vector(), nullptr);
					HitActor->TakeDamage(34, DamageEvent, Thief->GetController(), Thief);
				}
			}

			// Add Recoil To Camera
			float Recoil = -Thief->Rifle->GetRecoil();

			if (Thief->bAimingC)
			{
				Recoil *= 0.6;
			}

			float Pitch = FMath::FRandRange(0, Recoil * 1.5);
			float Yaw = FMath::FRandRange(-Recoil, Recoil);

			Thief->AddControllerPitchInput(Pitch * 2);
			Thief->AddControllerYawInput(Yaw);

			if(Thief->FireMontage)
				Thief->ThiefAnimInst->Montage_Play(Thief->FireMontage);

			// Start Thread to notify nearby Enemies and NPCs
			(new FAutoDeleteAsyncTask<FGunNotifier>(Thief->ActorsToNotify, Location))->StartBackgroundTask();

			UGameplayStatics::PlaySound2D(Thief->GetWorld(), Thief->ShootSound, Thief->PlayerSoundClass->Properties.Volume, RAND_PITCH(0.95f, 1.05f));
		}
	}
}

void UPlayerStateBase::StopShoot()
{
	// No Implementation
}

void UPlayerStateBase::Aim()
{
	// No Implementation
}

void UPlayerStateBase::StopAim()
{
	// No Implementation
}

void UPlayerStateBase::Jump()
{
	// No Implementation
}

void UPlayerStateBase::Crouch()
{
	if(Thief)
	{
		Manager->bWantsToCrouchOnLand = true;
	}
}

void UPlayerStateBase::ReleaseCrouch()
{
	if(Thief)
	{
		Manager->bWantsToCrouchOnLand = false;
	}
}

void UPlayerStateBase::CrouchOnLand()
{
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Thief);

	FVector Start = Thief->GetActorLocation();
	FVector End = Thief->GetActorLocation() + Thief->GetActorUpVector() * -100;

	if (Thief->GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat(0,0,0,0), ECollisionChannel::ECC_Pawn, FCollisionShape::MakeSphere(22), Params))
	{
		if (Thief->GetVelocity().Size() > 601)
		{
			Manager->SwitchNextTickByKey(SLIDE);
		}
		else
		{
			Manager->SwitchNextTickByKey(CROUCH);
		}
	}
}

void UPlayerStateBase::Reload()
{
	if(Thief)
		Thief->Rifle->Reload();
}

void UPlayerStateBase::Sprint()
{
	if (Thief)
		Thief->GetCharacterMovement()->MaxWalkSpeed = Thief->DefaultMaxWalkSpeed * Thief->SprintSpeedMultiplier;
}

void UPlayerStateBase::StopSprint()
{
	if (Thief)
		Thief->GetCharacterMovement()->MaxWalkSpeed = Thief->DefaultMaxWalkSpeed;
}

void UPlayerStateBase::ControllerSprint()
{
	if(Thief && !Manager->bControllerSprintOn)
	{
		Manager->bControllerSprintOn = true;
		Manager->TimeSinceSprintToggle = 0;
		Thief->GetCharacterMovement()->MaxWalkSpeed = Thief->DefaultMaxWalkSpeed * Thief->SprintSpeedMultiplier;
		// GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Orange, "Toggle Sprint ON");
	}
}

void UPlayerStateBase::ReleaseControllerSprint()
{
}

void UPlayerStateBase::PlayerLanded(const FHitResult& Hit)
{
	if (Thief->LandedSound)
		UGameplayStatics::PlaySound2D(Thief->GetWorld(), Thief->LandedSound, Thief->PlayerSoundClass->Properties.Volume, RAND_PITCH(0.98f, 1.02f));
}

bool UPlayerStateBase::JumpAssistCheck()
{
	return FramesSinceGrounded < AllowedFramesForAssist;
}

bool UPlayerStateBase::Vault()
{
	if (Thief->VaultComponent->CharacterCanVault(Thief->VaultTargetXYZLocation))
	{
		Thief->bVaulting = true;
		Thief->bVaultingZ = true;

		Thief->VaultTargetZLocation = Thief->GetActorLocation();
		Thief->VaultTargetZLocation.Z = Thief->VaultTargetXYZLocation.Z;

		UGameplayStatics::PlaySound2D(Thief->GetWorld(), Thief->VaultSound, Thief->PlayerSoundClass->Properties.Volume, RAND_PITCH(0.98f, 1.02f));

		return true;
	}
	return false;
}

void UPlayerStateBase::OnStateEnter()
{
	bCanTick = true;

	UE_LOG(LogTemp, Warning, TEXT("Thief Entering State: %s"), *StateName.ToString());
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3, FColor::Green, "Thief Entering State: " + StateName.ToString());
}

void UPlayerStateBase::OnStateExit()
{
	bCanTick = false;

	UE_LOG(LogTemp, Log, TEXT("Thief Exiting State: %s"), *StateName.ToString());
	//GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3, FColor::Red, "Thief Exiting State: " + StateName.ToString());
}
