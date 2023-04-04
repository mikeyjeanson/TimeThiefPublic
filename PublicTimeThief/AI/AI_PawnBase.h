#pragma once

#include "CoreMinimal.h"
#include "Components/ArrowComponent.h"
#include "Components/SphereComponent.h"
#include "ProjectTimeThief/AI/Movement/NonPlayerCharacterMovement.h"
#include "ProjectTimeThief/AI/Navigation/NavPath.h"
#include "ProjectTimeThief/GunBase.h"
#include "ProjectTimeThief/AI/Base/BaseSword.h"
#include "AI_PawnBase.generated.h"

class UAI_Brain;
class UAI_StateManager;
class UAIPerceptionComponent;

UENUM(BlueprintType)
namespace EAISpeeds
{
	enum EType
	{
		Zero,
		Walk,
		FastWalk,
		Jog,
		Sprint,
		FastSprint
	};
}

UENUM(BlueprintType)
namespace EEnemyVisibility
{
	enum EType
	{
		NotVisible	UMETA(DisplayName = "Not Visible"),
		Visible		UMETA(DisplayName = "Visible")
	};
}

UENUM(BlueprintType)
namespace EControllerStatus
{
	enum EType
	{
		None,
		Sleep, // Sleeps the Controller
		Basic, // Basic No Perception and Slower Updates on Controller
		Normal // Normal Function as Normal Controller
	};
}

UENUM(BlueprintType)
namespace ECombatType
{
	enum EType
	{
		None,
		Melee,
		RangedClose,
		RangedFar
	};
}


UCLASS()
class PROJECTTIMETHIEF_API AAI_PawnBase : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAI_PawnBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly)
	UNonPlayerCharacterMovement* MovementComponent;

	UPROPERTY()
	class AAI_ControllerBase* AIController;

	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	UAI_StateManager* StateManager;

	UPROPERTY(EditDefaultsOnly)
	USkeletalMeshComponent* MeshComponent;

	UPROPERTY(EditDefaultsOnly)
	UCapsuleComponent* CapsuleComponent;

	UPROPERTY(EditDefaultsOnly)
	UArrowComponent* ArrowComponent;

	UPROPERTY(EditDefaultsOnly)
	TArray<UAnimMontage*> AttackMontages;

	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* ShootMontage;

	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* ReloadMontage;

	UPROPERTY()
	UAnimInstance* AnimInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Patrol")
	bool bIsLeader = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true), Category = "Patrol")
	ANavPath* Path;

	UPROPERTY(EditAnywhere, Category = "Movement Speeds")
	float WalkSpeed = 200.f;
	UPROPERTY(EditAnywhere, Category = "Movement Speeds")
	float FastWalkSpeed = 300.f;
	UPROPERTY(EditAnywhere, Category = "Movement Speeds")
	float JogSpeed = 450.f;
	UPROPERTY(EditAnywhere, Category = "Movement Speeds")
	float SprintSpeed = 600.f;
	UPROPERTY(EditAnywhere, Category = "Movement Speeds")
	float FastSprintSpeed = 800.f;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	FORCEINLINE void SetAIController(AAI_ControllerBase* SetController) { AIController = SetController; }
	FORCEINLINE AAI_ControllerBase* GetAIController() const { return AIController; }
	FORCEINLINE void SetIsLeader(bool Value) { bIsLeader = Value; }
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE UAI_StateManager* GetStateManager() const { return StateManager; }
	FORCEINLINE void SetPath(ANavPath* SetPath) { Path = SetPath; }
	FORCEINLINE ANavPath* GetPath() const { return Path; }

	FORCEINLINE float GetWalkSpeed() const { return WalkSpeed; }
	FORCEINLINE float GetFastWalkSpeed() const { return FastWalkSpeed; }
	FORCEINLINE float GetJogSpeed() const { return JogSpeed; }
	FORCEINLINE float GetSprintSpeed() const { return SprintSpeed; }
	FORCEINLINE float GetFastSprintSpeed() const { return FastSprintSpeed; }

	virtual UPawnMovementComponent* GetMovementComponent() const override;
	UFUNCTION(BlueprintCallable)
	FORCEINLINE UNonPlayerCharacterMovement* GetNonPlayerCharacterMovement() const { return MovementComponent; }

	// Is the AI playing catchup (i.e. far behind desired location)
	UPROPERTY(EditAnywhere, Category = "Patrol")
	bool bCatchup = false;
	UPROPERTY(EditAnywhere, Category = "Patrol")
	bool ReversePathDirection = false;

	// Scores
	FName HealthKey = FName("Health");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scores|Health")
	float Health = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scores|Health")
	float MaxHealth = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scores|Health")
	float MinHealth = 0;

	FName FearKey = FName("Fear");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scores|Fear")
	float Fear = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scores|Fear")
	float MaxFear = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scores|Fear")
	float MinFear = 0;

	FName ConfidenceKey = FName("Confidence");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scores|Confidence")
	float Confidence = 50;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scores|Confidence")
	float MaxConfidence = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scores|Confidence")
	float MinConfidence = 0;

	FName SusKey = FName("Suspicion");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scores|Suspicion")
	float Suspicion = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scores|Suspicion")
	float MaxSuspicion = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scores|Suspicion")
	float MinSuspicion = 0;

	FORCEINLINE FVector GetLastStimuliLocation() const { return LastStimuliLocation; }
	FORCEINLINE void SetLastStimuliLocation(FVector const Location) { LastStimuliLocation = Location; }

	FORCEINLINE void ResetTimeSinceDestroy() { TimeSinceDestroy = -1.f; }
	FORCEINLINE void ZeroTimeSinceDestroy() { TimeSinceDestroy = 0.f; }

	FORCEINLINE USphereComponent* GetNotifier() const { return Notifier; }
	FORCEINLINE TSet<AAI_PawnBase*> GetNotifiedActors() const { return NotifiedActors; }

	// Tell AI to respond to given location
	virtual void RespondTo(FVector RespondToLocation);
	FORCEINLINE FVector GetRespondLocation() const { return RespondLocation; }

	FORCEINLINE void SetTargetHostile(AActor* Hostile) { TargetHostile = Hostile; }
	FORCEINLINE AActor* GetTargetHostile() const { return TargetHostile; }
	FORCEINLINE void ForgetTargetHostile() { TargetHostile = nullptr; }

	// Turn off or on thinking(Behavior Tree) and set AI's ControllerStatus(Sleep, Basic, Normal)
	// takes effect on next tick
	virtual void SetEnableThinking(const bool bSet, const TEnumAsByte<EControllerStatus::EType> ControllerStatus = EControllerStatus::Sleep);
	// Turn off or on animation rendering
	// takes effect on next tick
	virtual void SetEnableRendering(const bool bSet);

	FORCEINLINE bool IsThinking() const { return bIsThinking; }
	FORCEINLINE bool IsRendering() const { return bIsRendering; }

	// Changes Thinking Status and Controller Status
	virtual void ChangeThinkingStatus(bool bSet, const TEnumAsByte<EControllerStatus::EType> ControllerStatus = EControllerStatus::Sleep);
	// Changes Rendering Status
	virtual void ChangeRenderingStatus(bool bSet);

	FORCEINLINE bool IsPossessed() const { return bIsPossessed; }
	FORCEINLINE void SetIsPossessed(bool bSet) { bIsPossessed = bSet; }

	FORCEINLINE TEnumAsByte<EControllerStatus::EType> GetControllerStatus() const { return ControllerStatusEnum; }
	FORCEINLINE void SetControllerStatus(const TEnumAsByte<EControllerStatus::EType> Status) { ControllerStatusEnum = Status; }

protected:
	TEnumAsByte<EControllerStatus::EType> ControllerStatusEnum = EControllerStatus::None;

	bool bChangeThinkingStatusOnTick = false;
	bool bChangeRenderingStatusOnTick = false;

	bool bSetThinkingStatusTo = false;
	bool bSetRenderingStatusTo = false;

	bool bIsThinking = false;
	bool bIsRendering = false;

	TEnumAsByte<EControllerStatus::EType> ControllerStatusToSet = EControllerStatus::None;

	bool bIsPossessed = false;

	UPROPERTY(EditAnywhere, Category = "Search")
	USphereComponent* Notifier;

	UPROPERTY()
	AActor* TargetHostile;

	UFUNCTION()
	virtual void OnNotifierBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnNotifierEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	TSet<AAI_PawnBase*> NotifiedActors;

	FVector RespondLocation;

	FVector LastStimuliLocation;

	// Time since the AI was in the destroy state
	float TimeSinceDestroy = -1.f;

	UPROPERTY(EditAnywhere)
	float TimeToForgetDestroy = 45.f;

	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	TEnumAsByte<ECombatType::EType> CombatType = ECombatType::None;

	UPROPERTY()
	ABaseSword* Sword;

	UPROPERTY()
	AGunBase* Gun;

	UPROPERTY(EditDefaultsOnly)
	int32 WeaponAccuracy = 75;

	
public:
	FORCEINLINE TEnumAsByte <ECombatType::EType> GetCombatType() const { return CombatType; }
	FORCEINLINE void SetCombatType(const TEnumAsByte<ECombatType::EType> NewType) { CombatType = NewType; }

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual void BeginToDie(FVector DirectionToDie, FVector HitLocation);

	bool bIsDead = false;
	bool bIsShepherd = false;

	UPROPERTY()
	class AAI_SpawnerBase* Spawner = nullptr;

	FTimerHandle StopRagdollTimerHandle;
	FTimerHandle DestroyAfterDeathTimerHandle;

	void StopRagdoll();
	void StartDestroyTimer();
	void DestroyAfterDeath();

	UFUNCTION(BlueprintCallable)
	virtual void Attack();

	UFUNCTION(BlueprintCallable)
	virtual void Defend();

public:
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundClass* SoundClass;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundAttenuation* AttenuationClass;

	UFUNCTION(BlueprintPure)
	FORCEINLINE USoundClass* GetSoundClass() const { return SoundClass; }

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TArray<USoundWave*> SwordSoundArray;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TArray<USoundWave*> DamageSounds;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TArray<USoundWave*> DeathSounds;

	// State Specific Sounds

	UPROPERTY(EditDefaultsOnly, Category = "Sound|States")
	TArray<USoundWave*> PatrolSounds;

	UPROPERTY(EditDefaultsOnly, Category = "Sound|States")
	TArray<USoundWave*> SearchSounds;

	UPROPERTY(EditDefaultsOnly, Category = "Sound|States")
	TArray<USoundWave*> DestroySounds;

	UPROPERTY(EditDefaultsOnly, Category = "Sound|States")
	TArray<USoundWave*> MiscSounds;
};

//=========================================

class FAI_PawnNotifier : public FNonAbandonableTask
{

public:
	TSet<AAI_PawnBase*> CharactersToNotify;
	FVector NotifyLocation;

	FAI_PawnNotifier(TSet<AAI_PawnBase*> Characters, FVector ActorLocation);
	~FAI_PawnNotifier();

	FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(AI_CharacterNotifier, STATGROUP_ThreadPoolAsyncTasks); }
	void DoWork() const;
};