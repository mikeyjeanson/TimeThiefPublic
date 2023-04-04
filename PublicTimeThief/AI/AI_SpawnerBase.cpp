#include "AI_SpawnerBase.h"
#include "NavigationSystem.h"
#include "Components/BillboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SpawnerController.h"
#include "ProjectTimeThief/AI/NPC/AI_NPCController.h"

// Sets default values
AAI_SpawnerBase::AAI_SpawnerBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule Component"));
	CapsuleComponent->SetCapsuleRadius(36);
	CapsuleComponent->SetCapsuleHalfHeight(90);
	RootComponent = CapsuleComponent;

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	BillboardComponent->SetComponentTickEnabled(false);
	BillboardComponent->SetupAttachment(RootComponent);

	VisualSphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere Component"));
	VisualSphereComponent->SetSphereRadius(500);
	VisualSphereComponent->SetComponentTickEnabled(false);
	VisualSphereComponent->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AAI_SpawnerBase::BeginPlay()
{
	Super::BeginPlay();

	// Get Level Controller If Null
	if(LevelController == nullptr)
	{
		TArray<AActor*> OutActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAI_LevelController::StaticClass(), OutActors);

		for(AActor* Actor : OutActors)
		{
			LevelController = Cast<AAI_LevelController>(Actor);
		}
	}

	// Get NPC Controller If Null
	if (NPCController == nullptr)
	{
		TArray<AActor*> OutActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANPC_LevelController::StaticClass(), OutActors);

		for (AActor* Actor : OutActors)
		{
			NPCController = Cast<ANPC_LevelController>(Actor);
		}
	}

	if (!IsValid(Shepherd))
		SpawnShepherd();

	if(VisualSphereComponent)
	{
		Radius = VisualSphereComponent->GetScaledSphereRadius();
		VisualSphereComponent->DestroyComponent();
	}
}

/**
 * 
 * @warning This Method runs on a BACKGROUND THREAD and must be thread safe
 */
bool AAI_SpawnerBase::FindValidSpawnLocation(FVector& OutLocation, FRotator& OutRotation)
{
	if (Shepherd)
	{
		if(const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Shepherd->GetComponentByClass(UCapsuleComponent::StaticClass())); IsValid(Capsule))
		{
			bool bSpawnLocationFound = false;
			const float AdjustedHalfHeight = Capsule->GetScaledCapsuleHalfHeight() - 10;

			FNavLocation NavLocation;
			const UNavigationSystemV1* Navigation = UNavigationSystemV1::GetCurrent(Shepherd->GetWorld());

			while(!bSpawnLocationFound)
			{
				// Get Spot
				if (Navigation && Navigation->GetRandomPointInNavigableRadius(GetActorLocation(), Radius, NavLocation))
				{
					// Check If Not In Another Object
					DrawDebugSphere(GetWorld(), NavLocation.Location, 20, 8, FColor::Cyan);

					FVector Start = NavLocation.Location + FVector::UpVector * AdjustedHalfHeight;
					FVector End = Start + FVector(0,0,0.1f);
					FQuat Quat = FQuat(Start, 0);

					bSpawnLocationFound = !GetWorld()->SweepSingleByChannel(Hit, Start, End, Quat, ECollisionChannel::ECC_Pawn,
						FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), AdjustedHalfHeight));

					DrawDebugCapsule(GetWorld(), End, Capsule->GetScaledCapsuleRadius(),
									 AdjustedHalfHeight, Quat, FColor::Magenta, false, 3);

					OutLocation = NavLocation.Location + (Capsule->GetScaledCapsuleHalfHeight() * FVector(0, 0, 1.2));
				}
			}
			OutRotation = FRotator(0, FMath::RandRange(-180, 180), 0);

			return true;
		}
	}
	return false;
}

void AAI_SpawnerBase::SpawnShepherd()
{
	Shepherd = GetWorld()->SpawnActor<APawn>(ActorToSpawn, GetActorLocation(), GetActorRotation());

	// Add Shepherd to overhead controller
	if (AAI_PawnBase* AI = Cast<AAI_PawnBase>(Shepherd); IsValid(AI))
	{
		LevelController->AddAIToThreadPool(AI);

		if (Path)
			AI->SetPath(Path);

		AI->bIsShepherd = true;
		AI->Spawner = this;
	}
	else if (AAAI_NPCBase* NPC = Cast<AAAI_NPCBase>(Shepherd); IsValid(NPC))
	{
		NPCController->AddNPCToThreadPool(NPC);

		if (Path)
			NPC->SetPath(Path);
	}
}

void AAI_SpawnerBase::SpawnAll()
{
	uint8 Counter = SheepCount;
	while (Counter --> 0)
	{
		//(new FAutoDeleteAsyncTask<FAI_SpawnerThread>(this))->StartBackgroundTask();
		PrepareSpawn();
	}
}

bool AAI_SpawnerBase::DespawnAll()
{
	TArray<APawn*> SheepArray = Sheep.Array();

	for (APawn* Actor : SheepArray)
	{
		if (AAAI_NPCBase* NPC = Cast <AAAI_NPCBase>(Actor); IsValid(NPC))
			NPCController->RemoveNPCFromThreadPool(NPC);
		else if (AAI_PawnBase* Pawn = Cast<AAI_PawnBase>(Actor); IsValid(Pawn))
			LevelController->RemoveAIFromThreadPool(Pawn);

		Sheep.Remove(Actor);
	}
	Sheep.Reset();
	return true;
}


void AAI_SpawnerBase::PrepareSpawn()
{
	FSpawnPoint SpawnPoint;

	if(FindValidSpawnLocation(SpawnPoint.Location, SpawnPoint.Rotation))
	{
		const FSpawnRequest Request = FSpawnRequest(SpawnPoint, this);

		SpawnController->Mutex.Lock();
		SpawnController->SpawnRequests.Enqueue(Request);
		SpawnController->Mutex.Unlock();
	}
}

void AAI_SpawnerBase::DespawnByPointer(APawn* Pointer)
{
	if (AAAI_NPCBase* NPC = Cast <AAAI_NPCBase>(Pointer); IsValid(NPC))
		NPCController->RemoveNPCFromThreadPool(NPC);
	else if (AAI_PawnBase* Pawn = Cast<AAI_PawnBase>(Pointer); IsValid(Pawn))
		LevelController->RemoveAIFromThreadPool(Pawn);

	Sheep.Remove(Pointer);
}

bool AAI_SpawnerBase::IsFinishedSpawning() const
{
	return SheepCount == Sheep.Num();
}

bool AAI_SpawnerBase::IsFinishedDespawning() const
{
	return Sheep.Num() == 0;
}

APawn* AAI_SpawnerBase::SpawnAtLocation(const FSpawnPoint& SpawnPoint)
{
	APawn* SpawnedActor = GetWorld()->SpawnActor<APawn>(ActorToSpawn, SpawnPoint.Location, SpawnPoint.Rotation);

	if (!IsValid(SpawnedActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("Spawn Actor Failed"));

		return nullptr;
	}

	Mutex.Lock();
	{
		Sheep.Emplace(SpawnedActor);
	}
	Mutex.Unlock();

	// TODO: Run Final Setup i.e. Path Setup + Blackboard Values
	
	if (AAI_PawnBase* AI = Cast<AAI_PawnBase>(SpawnedActor))
	{
		if (Path)
			AI->SetPath(Path);

		LevelController->AddAIToThreadPool(AI);

		AI->Spawner = this;
	}
	else if (AAAI_NPCBase* NPC = Cast<AAAI_NPCBase>(SpawnedActor))
	{
		if (Path)
			NPC->SetPath(Path);

		NPCController->AddNPCToThreadPool(NPC);
	}

	return SpawnedActor;
}

void AAI_SpawnerBase::KillSheepByPtr(APawn* SheepPtr)
{
	if(Sheep.Contains(SheepPtr))
	{
		Sheep.Remove(SheepPtr);
		SheepCount--;

		if(SheepCount == 0 && Shepherd == nullptr || (Shepherd->GetClass() == AAI_PawnBase::StaticClass() && Cast<AAI_PawnBase>(Shepherd)->bIsDead))
		{
			bAllDead = true;
			SpawnController->RemoveSpawner(this);
		}
	}
}

void AAI_SpawnerBase::FindNewShepherd()
{
	Shepherd = nullptr;

	for(APawn* Pawn : Sheep)
	{
		if(AAI_PawnBase* PawnBase = Cast<AAI_PawnBase>(Pawn); IsValid(PawnBase) && !PawnBase->bIsDead)
		{
			PawnBase->bIsShepherd = true;
			Shepherd = PawnBase;

			Sheep.Remove(Shepherd);
			SheepCount--;

			return;
		}
	}
}

//================================

FAI_SpawnerThread::FAI_SpawnerThread(AAI_SpawnerBase* SpawnerBase)
{
	Spawner = SpawnerBase;
}

FAI_SpawnerThread::~FAI_SpawnerThread()
{
	// Do Nothing
}

void FAI_SpawnerThread::DoWork() const
{
	Spawner->PrepareSpawn();
}


