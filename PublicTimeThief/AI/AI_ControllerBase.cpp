// Fill out your copyright notice in the Description page of Project Settings.


#include "AI_ControllerBase.h"
#include "AI_StateManager.h"
#include "../Base/AI_PawnBase.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "ProjectTimeThief/Thief/Thief.h"
#include "States/Substates/AI_SubstateBase.h"

AAI_ControllerBase::AAI_ControllerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>("Behavior Tree Component");
	Blackboard = CreateDefaultSubobject<UBlackboardComponent>("Blackboard Component");
	Perception = CreateDefaultSubobject<UAIPerceptionComponent>("AI Perception");
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>("Sight");

	// Set Sight Properties
	SightConfig->SightRadius = 2000.f;
	SightConfig->LoseSightRadius = 2000.f;

	Perception->ConfigureSense(*SightConfig);
	Perception->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AAI_ControllerBase::BeginPlay()
{
	Super::BeginPlay();

	if(IsValid(BTAsset))
	{
		RunBehaviorTree(BTAsset);
		BehaviorTreeComponent->StartTree(*BTAsset);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BTAsset NOT VALID"));
	}

	if (Perception)
		Perception->OnPerceptionUpdated.AddDynamic(this, &AAI_ControllerBase::PerceptionUpdated);

	// Set Crowd Following Component
	if (CrowdFollowingComponent = Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()); IsValid(CrowdFollowingComponent))
	{
		CrowdFollowingComponent->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Good, true);
		CrowdFollowingComponent->SetCrowdCollisionQueryRange(1000.f, true);
		CrowdFollowingComponent->SetCrowdAvoidanceRangeMultiplier(1.0f, true);
	}
}

void AAI_ControllerBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	PerceiveCurrentReality(DeltaSeconds);
}

void AAI_ControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledCharacter = Cast<AAI_PawnBase>(InPawn);
	ControlledCharacter->SetAIController(this);
	ControlledCharacter->SetIsPossessed(true);

	// Setup Blackboard
	if (IsValid(BTAsset) && IsValid(Blackboard))
	{
		Blackboard->InitializeBlackboard(*BTAsset->BlackboardAsset);
	}

	if(ANavPath* Path = ControlledCharacter->GetPath(); Path != nullptr)
	{
		Blackboard->SetValueAsObject(FName("Path"), Path);
	}

	const ECombatType::EType CombatType = ControlledCharacter->GetCombatType();
	Blackboard->SetValueAsEnum(FName("Combat Type"), CombatType);

	GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Cyan, (TEXT("%s Possessed~~~~~~~"), *ControlledCharacter->GetActorNameOrLabel()));
}

void AAI_ControllerBase::OnUnPossess()
{
	ControlledCharacter->SetIsPossessed(false);
	GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Orange, (TEXT("%s UnPossessed!!!!"), *ControlledCharacter->GetActorNameOrLabel()));

	Super::OnUnPossess();
}

void AAI_ControllerBase::PerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
	for(AActor* Actor : UpdatedActors)
	{
		if (const AThief* Thief = Cast<AThief>(Actor); IsValid(Thief))
		{
			if (KnownHostileActors.Contains(Actor))
			{
				// Actor Has Left Vision
				KnownHostileActors.Remove(Actor);
				OutOfSightHostiles.Add(Actor);

				// Calculate and Set the Estimated Updated Character Location
				FHitResult Hit;
				FQuat Quat = FQuat(Actor->GetActorForwardVector(), 0);
				FCollisionQueryParams Params;
				Params.AddIgnoredActor(Actor);

				const bool bHit = GetWorld()->SweepSingleByChannel(
					Hit,
					Actor->GetActorLocation(),
					Actor->GetActorLocation() + Actor->GetVelocity() * 1.f,
					Quat,
					ECollisionChannel::ECC_Pawn,
					FCollisionShape::MakeCapsule(
						(Thief->GetCapsuleComponent()->GetScaledCapsuleRadius() - 1),
						(Thief->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()) - 5),
					Params);

				GuessLocation = bHit ? Hit.Location : Actor->GetActorLocation();
			}
			else
			{
				// Actor Has Entered Vision

				// Forget Actor if they are outside the Real Sight Radius when they enters vision
				if (FVector::Dist2D(Actor->GetActorLocation(), ControlledCharacter->GetActorLocation()) > RealSightRadius)
				{
					Perception->ForgetActor(Actor);
					continue;
				}

				GuessLocation = Actor->GetActorLocation();
				KnownHostileActors.Add(Actor);

				ControlledCharacter->SetTargetHostile(Actor);

				if (OutOfSightHostiles.Contains(Actor))
				{
					OutOfSightHostiles.Remove(Actor);
				}
			}
			if (ControlledCharacter->GetStateManager()->GetCurrentState() != AI_State::Destroy)
			{
				LastSuspiciousActorSensed = Actor;
			}
		}
		else if(Actor->GetClass() == GetClass())
		{
			ControlledCharacter->Suspicion += SuspicionUpRate * 0.06f;
			ControlledCharacter->Suspicion = FMath::Clamp(ControlledCharacter->Suspicion,
				ControlledCharacter->MinSuspicion, ControlledCharacter->MaxSuspicion);
		}
	}
}

void AAI_ControllerBase::PerceiveCurrentReality(float DeltaTime)
{
	if (!KnownHostileActors.IsEmpty())
	{
		for (AActor* Actor : KnownHostileActors)
		{
			if (const AThief* Thief = Cast<AThief>(Actor); IsValid(Thief))
			{
				// Raise Suspicion
				const FVector UpdatedCharacterLocation = Thief->GetActorLocation();
				const FVector ControlledCharacterLocation = ControlledCharacter->GetActorLocation();

				const float DistanceFromCharacter = FVector::Dist(UpdatedCharacterLocation, ControlledCharacterLocation);
				float DistanceFactor = 1.f;

				if (DistanceFromCharacter < 400)
				{
					// Inside 4 Meters
					DistanceFactor = 3.f;
				}
				else if (DistanceFromCharacter < 800)
				{
					// Inside 8 Meters
					DistanceFactor = 1.5f;
				}

				ControlledCharacter->Suspicion += SuspicionUpRate * DeltaTime * DistanceFactor;
				ControlledCharacter->Suspicion = FMath::Clamp(ControlledCharacter->Suspicion,
					ControlledCharacter->MinSuspicion, ControlledCharacter->MaxSuspicion);

				GuessLocation = Actor->GetActorLocation();
			}
			else if (Actor->GetClass() == ControlledCharacter->GetClass())
			{
				ControlledCharacter->Suspicion += SuspicionUpRate * DeltaTime;
				ControlledCharacter->Suspicion = FMath::Clamp(ControlledCharacter->Suspicion,
					ControlledCharacter->MinSuspicion, ControlledCharacter->MaxSuspicion);
			}
		}

		if(const UAI_StateBase* SearchState = ControlledCharacter->GetStateManager()->GetSpecificState(AI_State::Search); IsValid(SearchState))
		{
			SearchState->GetSubstatePtr()->SwitchSubstate(ESubstate::Tracking);
		}
	}

	if(KnownHostileActors.IsEmpty())
	{
		// Decrease Suspicion 
		ControlledCharacter->Suspicion -= SuspicionDownRate * DeltaTime;
		ControlledCharacter->Suspicion = FMath::Clamp(ControlledCharacter->Suspicion,
			ControlledCharacter->MinSuspicion, ControlledCharacter->MaxSuspicion);
	}
}

void AAI_ControllerBase::TargetPerceptionUpdated(AActor* TargetActor, FAIStimulus Stimulus)
{
	if (TargetActor->GetClass() == ControlledCharacter->GetClass())
	{

	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red,
			"Target Location Updated -> " + TargetActor->GetActorNameOrLabel());
	}
}

void AAI_ControllerBase::TargetInfoUpdated(const FActorPerceptionUpdateInfo& UpdateInfo)
{
	if(const AActor* TargetActor = UpdateInfo.Target.Get(); IsValid(TargetActor))
	{
		if (TargetActor->GetClass() == ControlledCharacter->GetClass())
		{
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red,
				"Target Location Updated -> " + TargetActor->GetActorNameOrLabel());
		}
	}
}

/**
 * @param bSet Is False if Controller Status does not equal: EControllerStatus::Normal
 * @param ControllerStatus Status of the AI Controller to Set
 */
void AAI_ControllerBase::SetEnableThinking(const bool bSet, const TEnumAsByte<EControllerStatus::EType> ControllerStatus)
{
	// Set All Components Status
	Perception->SetSenseEnabled(SightConfig->GetSenseImplementation(), bSet);
	Perception->SetComponentTickEnabled(bSet);
	Perception->SetActive(bSet, true);

	// Set Blackboard Value for AI Controller
	Blackboard->SetValueAsEnum(FName("Controller Status"), ControllerStatus);
	ControlledCharacter->SetControllerStatus(ControllerStatus);

	// Specific Adjustments
	switch (ControllerStatus)
	{
		case EControllerStatus::Sleep:
			break;
		case EControllerStatus::Basic:
			// Adjust Perception Tick Rate
			Perception->SetComponentTickInterval(PerceptionBasicTickRate);
			break;
		case EControllerStatus::Normal:
			// Adjust Perception Tick Rate
			Perception->SetComponentTickInterval(PerceptionNormalTickRate);
			break;
		default:
			break;
	}
}

void AAI_ControllerBase::SetBlackboardVectorWithKey(FVector& Vector, FName Key)
{
	Blackboard->SetValueAsVector(Key, Vector);
}

void AAI_ControllerBase::SetBlackboardObjectWithKey(UObject* Object, FName Key)
{
	Blackboard->SetValueAsObject(Key, Object);
}
