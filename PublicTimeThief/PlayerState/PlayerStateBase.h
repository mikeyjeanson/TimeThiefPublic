#pragma once

#include "CoreMinimal.h"
#include "../Thief.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/NoExportTypes.h"
#include "../VaultComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "PlayerStateBase.generated.h"

// Define State Keys
#define BASE 0
#define WALK 1
#define CROUCH 2
#define SPRINT 3
#define VAULT 4
#define AIR 5
#define WALLRUN 6
#define SLIDE 7

// Random Pitch macro sounds
#define RAND_PITCH(s, a) FMath::RandRange(s, a)

/**
 * Player State Base for Player State Manager
 * Handles Player Input Based On CurrentState
 */
UCLASS()
class PROJECTTIMETHIEF_API UPlayerStateBase : public UObject
{
	GENERATED_BODY()
	
public:

	// Default Constructor
	UPlayerStateBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName StateName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int StateID;

	bool bCanTick = false;
	bool bCanRepeat = false;

	UPROPERTY()
	UPlayerStateManager* Manager;

	// Character & Movement
	UPROPERTY(VisibleAnywhere)
	AThief* Thief;

	virtual void StateTick(float DeltaTime);

	// Input Functions
	virtual void MoveForward(float AxisValue);
	virtual void LookUp(float AxisValue);
	virtual void MoveRight(float AxisValue);
	virtual void LookRight(float AxisValue);

	// Controller
	virtual void LookUpRate(float AxisValue);
	virtual void LookRightRate(float AxisValue);

	// Actions
	virtual void Dodge();
	virtual void Jump();
	virtual void Crouch();
	virtual void ReleaseCrouch();
	virtual void CrouchOnLand();
	virtual void Reload();

	// Modifiers
	virtual void Sprint();
	virtual void StopSprint();

	virtual void ControllerSprint();
	virtual void ReleaseControllerSprint();

	// Shooting and Aiming
	virtual void Shoot();
	virtual void StopShoot();
	virtual void Aim();
	virtual void StopAim();

	// Landed
	virtual void PlayerLanded(const FHitResult& Hit);

	// On Enter and Exit
	virtual void OnStateEnter();
	virtual void OnStateExit();

protected:

	// Basic Movement
	UPROPERTY(EditAnywhere, Category = "Basic Movement")
	FVector FirstJumpImpulse = FVector(0, 0, 420);

	UPROPERTY(EditAnywhere, Category = "Basic Movement")
	FVector DoubleJumpImpulse = FVector(0, 0, 300);

	UPROPERTY(EditAnywhere, Category = "Basic Movement")
	float DodgeForceMultipier = 1500;

	// Wall Running
	UPROPERTY(EditAnywhere, Category = "Wall Running")
	FRotator WallRunningCameraRotation = FRotator(0, 0, 20);

	UPROPERTY(EditAnywhere, Category = "Wall Running")
	float WallRunningCameraInterpSpeed = 2.0;

	// Jump Assist
	int FramesSinceGrounded = 0;
	int AllowedFramesForAssist = 5;

	// True if TimeSinceGrouded < AllowedTimeForAssist
	bool JumpAssistCheck();

	// Shooting
	UPROPERTY(EditAnywhere)
	float MaxRange = 10000;

	//Vaulting
	bool Vault();
};
