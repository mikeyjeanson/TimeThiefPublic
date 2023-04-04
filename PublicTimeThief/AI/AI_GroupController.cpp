#include "AI_GroupController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AAI_GroupController::AAI_GroupController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AAI_GroupController::BeginPlay()
{
	Super::BeginPlay();

	int32 AvoidanceGroup32Bit = 0x0000;
	int32 GroupsToAvoid32Bit = 0xFFFF;


	if (AvoidanceGroup != 0)
	{
		AvoidanceGroup32Bit = 0x0001;

		// Set Avoidance Group
		AvoidanceGroup32Bit = AvoidanceGroup32Bit << (AvoidanceGroup - 1);

		// Set Other Groups with negation of Avoidance Groups
		GroupsToAvoid32Bit = ~AvoidanceGroup32Bit;
	}

	if (Leader != nullptr)
	{
		Leader->SetIsLeader(true);


		/*Leader does NOT ignore their own group*/
		// Commented Out Character Movement because TOO SLOW
		// Leader->GetCharacterMovement()->SetAvoidanceGroup(AvoidanceGroup32Bit);
		if (UPathFollowingComponent* PathFollowingComponent = Leader->GetAIController()->GetPathFollowingComponent(); IsValid(PathFollowingComponent))
		{
			if (UCrowdFollowingComponent* CrowdFollowingComponent = Cast<UCrowdFollowingComponent>(PathFollowingComponent); IsValid(CrowdFollowingComponent))
			{
				//CrowdFollowingComponent->SetAvoidanceGroup(AvoidanceGroup32Bit);
			}
		}

		// Set Path of the leader
		if (PatrolPath != nullptr)
		{
			Leader->SetPath(PatrolPath);
			if (AAI_ControllerBase* AIController = Leader->GetAIController(); IsValid(AIController))
			{
				AIController->SetBlackboardObjectWithKey(PatrolPath, FName("Path"));
				AIController->GetBlackboardComponent()->SetValueAsBool(FName("In Group"), true);
			}
		}
	}

	for(const AAI_PawnBase* Follower : Followers)
	{
		// Commented Out Character Movement because it is TOO SLOW
		/*Follower->GetCharacterMovement()->SetAvoidanceGroup(AvoidanceGroup32Bit);
		Follower->GetCharacterMovement()->SetGroupsToIgnore(AvoidanceGroup32Bit);
		Follower->GetCharacterMovement()->SetGroupsToAvoid(GroupsToAvoid32Bit);

		if (AAI_ControllerBase* AIController = Follower->GetAIController(); IsValid(AIController))
			AIController->GetBlackboardComponent()->SetValueAsBool(FName("In Group"), true);*/

		if (UPathFollowingComponent* PathFollowingComponent = Follower->GetAIController()->GetPathFollowingComponent(); IsValid(PathFollowingComponent))
		{
			if (UCrowdFollowingComponent* CrowdFollowingComponent = Cast<UCrowdFollowingComponent>(PathFollowingComponent); IsValid(CrowdFollowingComponent))
			{
				//CrowdFollowingComponent->SetAvoidanceGroup(AvoidanceGroup32Bit);
				//CrowdFollowingComponent->SetGroupsToIgnore(AvoidanceGroup32Bit);
				//CrowdFollowingComponent->SetGroupsToAvoid(GroupsToAvoid32Bit);

				if (AAI_ControllerBase* AIController = Follower->GetAIController(); IsValid(AIController))
					AIController->GetBlackboardComponent()->SetValueAsBool(FName("In Group"), true);
			}
		}
	}
}

// Called every frame
void AAI_GroupController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(Leader != nullptr)
		SetActorLocationAndRotation(Leader->GetActorLocation(), Leader->GetActorRotation());

	SetFormation();
}

void AAI_GroupController::SetFormation()
{
	if(Formation != GroupFormation::None)
	{
		const FVector Origin = GetActorLocation();
		//DrawDebugSphere(GetWorld(), Origin, 40, 8, FColor::Black);

		switch(Followers.Num())
		{
			case 1:
				SetFormationLeft(Followers[0], Origin);
				break;
			case 2:
				SetFormationLeft(Followers[0], Origin);
				SetFormationRight(Followers[1], Origin);
				break;
			case 3:
				SetFormationLeft(Followers[0], Origin);
				SetFormationRight(Followers[1], Origin);
				SetFormationBack(Followers[2], Origin);
				break;
			default:
				break;
		}
	}
}

void AAI_GroupController::SetFormationLeft(const AAI_PawnBase* Character, const FVector& Origin) const
{
	if(IsValid(Character))
	{
		FVector Point;

		switch(Formation)
		{
			case GroupFormation::Gentlemans:
				Point = Origin + GetActorForwardVector() * -FDistance;
				break;
			case GroupFormation::Triangle:
				Point = Origin + GetActorForwardVector() * -FDistance + GetActorRightVector() * -FHalfDistance;
				break;
			case GroupFormation::Diamond:
				Point = Origin + GetActorForwardVector() * -FDistance + GetActorRightVector() * -FHalfDistance;
				break;
			case GroupFormation::Box:
				Point = Origin + GetActorForwardVector() * -FBackDistance;
				break;
			default:
				break;
		}

		if(AAI_ControllerBase* AIController = Character->GetAIController(); IsValid(AIController))
			AIController->SetBlackboardVectorWithKey(Point, FName("Patrol Vector"));
	}
}

void AAI_GroupController::SetFormationRight(const AAI_PawnBase* Character, const FVector& Origin) const
{
	if(IsValid(Character))
	{
		FVector Point;

		switch (Formation)
		{
			case GroupFormation::Triangle:
				Point = Origin + GetActorForwardVector() * -FDistance + GetActorRightVector() * FHalfDistance;
				break;
			case GroupFormation::Diamond:
				Point = Origin + GetActorForwardVector() * -FDistance + GetActorRightVector() * FHalfDistance;
				break;
			case GroupFormation::Box:
				Point = Origin + GetActorRightVector() * FDistance + GetActorForwardVector() * FHalfDistance;
				break;
			default:
				break;
		}

		Character->GetAIController()->SetBlackboardVectorWithKey(Point, FName("Patrol Vector"));
	}
}

void AAI_GroupController::SetFormationBack(const AAI_PawnBase* Character, const FVector& Origin) const
{
	if(IsValid(Character))
	{
		FVector Point;

		switch (Formation)
		{
		case GroupFormation::Diamond:
			Point = Origin + GetActorForwardVector() * -FBackDistance;
			break;
		case GroupFormation::Box:
			Point = Origin + GetActorForwardVector() * -FBackDistance + GetActorRightVector() * FDistance;
			break;
		default:
			break;
		}

		Character->GetAIController()->SetBlackboardVectorWithKey(Point, FName("Patrol Vector"));
	}
}

void AAI_GroupController::AddFollower(AAI_PawnBase* Follower)
{
	Followers.Add(Follower);
}

void AAI_GroupController::RemoveFollower(AAI_PawnBase* Follower)
{
	if(Followers.Contains(Follower))
	{
		Followers.Remove(Follower);
	}
}

void AAI_GroupController::SetLeader(AAI_PawnBase* SetLeader)
{
	RemoveFollower(SetLeader);

	Leader = SetLeader;
}

