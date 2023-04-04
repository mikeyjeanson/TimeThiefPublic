// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_ShouldPatrolSpeedUp.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectTimeThief/AI/Base/AI_PawnBase.h"

UBTService_ShouldPatrolSpeedUp::UBTService_ShouldPatrolSpeedUp()
{
	NodeName = TEXT("Should Patrol Speed Up");

	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_ShouldPatrolSpeedUp, BlackboardKey));
}

FString UBTService_ShouldPatrolSpeedUp::GetStaticDescription() const
{
	return TEXT("Check distance of AI Path Length and adjusts walk speed if needed");
}

void UBTService_ShouldPatrolSpeedUp::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	// Check if the State has changed
	if (const AAIController* AIOwner = OwnerComp.GetAIOwner(); IsValid(AIOwner))
	{
		if (const UPathFollowingComponent* PathFollowing = AIOwner->GetPathFollowingComponent(); IsValid(PathFollowing))
		{
			if (const FNavPathSharedPtr Path = PathFollowing->GetPath(); Path.Get() != nullptr)
			{
				const float PathLength = Path->GetLength();

				if (AAI_PawnBase* Character = Cast<AAI_PawnBase>(AIOwner->GetPawn()); IsValid(Character))
				{
					UNonPlayerCharacterMovement* MovementComponent = Character->GetNonPlayerCharacterMovement();

					if (PathLength > MinDistanceToSpeedUp)
					{
						Character->bCatchup = true;

						if (PathLength > SprintDistance)
							MovementComponent->MaxSpeed = Character->GetSprintSpeed();
						else if (PathLength > JogDistance)
							MovementComponent->MaxSpeed = Character->GetJogSpeed();
						else
							MovementComponent->MaxSpeed = Character->GetFastWalkSpeed();
					}
					else if (Character->bCatchup)
					{
						if (PathLength < ResetDistance)
						{
							Character->bCatchup = false;
							MovementComponent->MaxSpeed = Character->GetWalkSpeed();
						}
					}
				}
			}
		}
	}
}
