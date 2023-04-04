// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "PlayerStateManager.h"
#include "PlayerStateBase.h"
#include "WalkState.h"
#include "CrouchState.h"
#include "SprintState.h"
#include "VaultState.h"
#include "AirState.h"
#include "WallRunState.h"
#include "SlideState.h"


	
// Sets default values for this component's properties
UPlayerStateManager::UPlayerStateManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	StateArray.Empty();

	InitAllStates();
}

// Called when the game starts
void UPlayerStateManager::BeginPlay()
{
	Super::BeginPlay();
	SwitchStateByKey(WALK);
	Thief = Cast<AThief>(GetOwner());
}

// Called every frame
void UPlayerStateManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(bSwitchNextTick)
	{
		bSwitchNextTick = false;
		SwitchStateByKey(NextStateKey);
	}

	if (!CurrentState->Thief)
	{
		CurrentState->Thief = Thief;
		UE_LOG(LogTemp, Warning, TEXT("Thief Reference: NULL POINTER"));
	}

	CurrentState->StateTick(DeltaTime);

	if(bShooting)
	{
		CurrentState->Shoot();
	}
}

UPlayerStateBase* UPlayerStateManager::GetCurrentState() const
{
	return CurrentState;
}

void UPlayerStateManager::InitAllStates()
{
	UPlayerStateBase* Base = CreateDefaultSubobject<UPlayerStateBase>("Base State");
	StateArray.Add(Base);

	UWalkState* Walk = CreateDefaultSubobject<UWalkState>("Walk State");
	StateArray.Add(Walk);

	UCrouchState* Crouch = CreateDefaultSubobject<UCrouchState>("Crouch State");
	StateArray.Add(Crouch);

	USprintState* Sprint = CreateDefaultSubobject<USprintState>("Sprint State");
	StateArray.Add(Sprint);

	UVaultState* Vault = CreateDefaultSubobject<UVaultState>("Vault State");
	StateArray.Add(Vault);

	UAirState* Air = CreateDefaultSubobject<UAirState>("Air State");
	StateArray.Add(Air);

	UWallRunState* Wall = CreateDefaultSubobject<UWallRunState>("Wall Run State");
	StateArray.Add(Wall);

	USlideState* Slide = CreateDefaultSubobject<USlideState>("Slide State");
	StateArray.Add(Slide);
}

void UPlayerStateManager::SwitchStateByKey(int StateKey)
{
	if (CurrentState)
	{
		switch (CurrentState->StateID)
		{
			case WALK:
				LastWalkOrCrouchState = false;
				break;

			case VAULT:
				if (StateKey == WALLRUN) return;
				break;

			case CROUCH:
				if (StateKey == SLIDE) return;
				if (StateKey == WALLRUN) return;
				LastWalkOrCrouchState = true;
				break;

			case WALLRUN:
				if (StateKey == SLIDE) return;
				break;

			case SLIDE:
				if (StateKey == WALLRUN) return;
				break;

			default:
				GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Red, "Switch State Switch Statement DID NOT WORK!!!");
		}
		LastState = CurrentState;
		CurrentState->OnStateExit();
	}
	CurrentState = StateArray[StateKey];
	CurrentState->Manager = this;
	CurrentState->Thief = Thief;
	CurrentState->OnStateEnter();
}

void UPlayerStateManager::SwitchNextTickByKey(int StateKey)
{
	NextStateKey = StateKey;
	bSwitchNextTick = true;
}

void UPlayerStateManager::MoveForward(float AxisValue)
{
	CurrentState->MoveForward(AxisValue);
}

void UPlayerStateManager::LookUp(float AxisValue)
{
	CurrentState->LookUp(AxisValue);
}

void UPlayerStateManager::MoveRight(float AxisValue)
{
	CurrentState->MoveRight(AxisValue);
}

void UPlayerStateManager::LookRight(float AxisValue)
{
	CurrentState->LookRight(AxisValue);
}

void UPlayerStateManager::LookUpRate(float AxisValue)
{
	CurrentState->LookUpRate(AxisValue);
}

void UPlayerStateManager::LookRightRate(float AxisValue)
{
	CurrentState->LookRightRate(AxisValue);
}

void UPlayerStateManager::Dodge()
{
	CurrentState->Dodge();
}

void UPlayerStateManager::Shoot()
{
	bShooting = true;
	CurrentState->Shoot();
}

void UPlayerStateManager::StopShoot()
{
	bShooting = false;
	CurrentState->StopShoot();
}

void UPlayerStateManager::Aim()
{
	CurrentState->Aim();
}

void UPlayerStateManager::StopAim()
{
	CurrentState->StopAim();
}

void UPlayerStateManager::Jump()
{
	CurrentState->Jump();
}

void UPlayerStateManager::Crouch()
{
	CurrentState->Crouch();
}

void UPlayerStateManager::ReleaseCrouch()
{
	CurrentState->ReleaseCrouch();
}

void UPlayerStateManager::Reload()
{
	CurrentState->Reload();
}

void UPlayerStateManager::Sprint()
{
	CurrentState->Sprint();
}

void UPlayerStateManager::StopSprint()
{
	CurrentState->StopSprint();
}

void UPlayerStateManager::ControllerSprint()
{
	CurrentState->ControllerSprint();
}

void UPlayerStateManager::ReleaseControllerSprint()
{
	CurrentState->ReleaseControllerSprint();
}

void UPlayerStateManager::PlayerLanded(const FHitResult& Hit)
{
	CurrentState->PlayerLanded(Hit);
}

