// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerStateManager.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTTIMETHIEF_API UPlayerStateManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPlayerStateManager();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	class UPlayerStateBase* GetCurrentState() const;

	// Switches state from CurrentState to new State with StateKey
	void SwitchStateByKey(int StateKey);

	// Setup State to switch next tick to new State with StateKey
	void SwitchNextTickByKey(int StateKey);

	UPROPERTY(BlueprintReadOnly)
	bool bShooting = false;

	UPlayerStateBase* LastState;

	// False for Walk, True for Crouch
	bool LastWalkOrCrouchState;

	bool bWantsToCrouchOnLand = false;

	float TimeSinceSprintToggle = 0;
	float SprintToggleNoCheckTime = 1;

private:

	// Setup all states and add them to State Array
	void InitAllStates();

	int NextStateKey;
	bool bSwitchNextTick = false;

	UPROPERTY(VisibleAnywhere)
	UPlayerStateBase* CurrentState;

	UPROPERTY(VisibleAnywhere)
	TArray<UPlayerStateBase*> StateArray;

	//////////// INPUT

	UPROPERTY(VisibleAnywhere)
	class AThief* Thief;

public:
	// Input Functions
	void MoveForward(float AxisValue);
	void LookUp(float AxisValue);
	void MoveRight(float AxisValue);
	void LookRight(float AxisValue);

	// Controller
	void LookUpRate(float AxisValue);
	void LookRightRate(float AxisValue);

	// Actions
	void Dodge();
	void Jump();
	void Crouch();
	void ReleaseCrouch();
	void Reload();

	// Modifiers
	void Sprint();
	void StopSprint();

	void ControllerSprint();
	void ReleaseControllerSprint();

	bool bControllerSprintOn = false;

	// Shooting and Aiming
	void Shoot();
	void StopShoot();
	void Aim();
	void StopAim();

	void PlayerLanded(const FHitResult& Hit);
};
