#include "WalkState.h"

UWalkState::UWalkState()
{
	StateName = TEXT("Walk State");
	StateID = WALK;
}

void UWalkState::StateTick(float DeltaTime)
{
	Super::StateTick(DeltaTime);

	if(Thief->CameraComponent)
	{
		if (!Thief->CameraComponent->GetRelativeRotation().IsNearlyZero())
		{
			Thief->CameraComponent->SetRelativeRotation(FMath::RInterpTo(Thief->CameraComponent->GetRelativeRotation(),
				FRotator::ZeroRotator, DeltaTime, Thief->WallRunningCameraInterpSpeed));
		}
		else
		{
			Thief->CameraComponent->SetRelativeRotation(FRotator::ZeroRotator);
		}
	}
}

void UWalkState::OnStateEnter()
{
	Super::OnStateEnter();
}

void UWalkState::OnStateExit()
{
	Super::OnStateExit();
}

void UWalkState::Jump()
{
	if (Vault())
	{
		Thief->bSingleJump = false;
		Thief->bDoubleJump = false;

		Thief->LastWall = nullptr;

		Manager->SwitchNextTickByKey(VAULT);

		return;
	}
	if (!Thief->bSingleJump && Thief->GetCharacterMovement()->IsMovingOnGround() || JumpAssistCheck())
	{
		Thief->bSingleJump = true;
		Thief->LaunchCharacter(FirstJumpImpulse, false, false);

		UGameplayStatics::PlaySound2D(Thief->GetWorld(), Thief->JumpSound, Thief->PlayerSoundClass->Properties.Volume, RAND_PITCH(0.98f, 1.02f));
	}
	else if (!Thief->bDoubleJump)
	{
		Thief->bDoubleJump = true;
		Thief->LaunchCharacter(DoubleJumpImpulse, false, true);

		UGameplayStatics::PlaySound2D(Thief->GetWorld(), Thief->DoubleJumpSound, Thief->PlayerSoundClass->Properties.Volume, RAND_PITCH(0.98f, 1.02f));
	}
}

void UWalkState::Crouch()
{
	Super::Crouch();

	if (Thief->GetCharacterMovement()->IsMovingOnGround())
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

void UWalkState::PlayerLanded(const FHitResult& Hit)
{
	Super::PlayerLanded(Hit);

	if(Manager->bWantsToCrouchOnLand)
	{
		CrouchOnLand();
	}
}
