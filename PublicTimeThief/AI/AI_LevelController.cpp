// Fill out your copyright notice in the Description page of Project Settings.


#include "AI_LevelController.h"
#include "ProjectTimeThief/AI/Spawners/SpawnerController.h"
#include "Kismet/GameplayStatics.h"

/**
 * @param PlayerPtr Player Pointer
 * @param AllActors Addresses of the AI Characters to look at
 * @param DefaultNumOfThinkingCharacters Number of AI Characters that should be thinking when closest to the Player
 * @param DefaultNumOfIntelligentCharacters Number of AI Characters that should be Intelligent when closest to the Player
 * @param TooFarAwayDistance Distance that the AI is too far away to render or think
 */
FAI_LevelControllerThread::FAI_LevelControllerThread(APawn* PlayerPtr, TArray<AAI_PawnBase*> AllActors,
	int DefaultNumOfThinkingCharacters, int DefaultNumOfIntelligentCharacters, float TooFarAwayDistance)
{
	Player = PlayerPtr;
	for(AAI_PawnBase* Character : AllActors)
		CharacterSet.Emplace(Character);
	
	NumOfThinkingCharacters = DefaultNumOfThinkingCharacters;
	NumOfIntelligentCharacters = DefaultNumOfIntelligentCharacters;
	TooFarAway = TooFarAwayDistance;
}


bool FAI_LevelControllerThread::Init()
{
	bStopThread = false;
	return true;
}

uint32 FAI_LevelControllerThread::Run()
{
	while(!bStopThread)
	{
		// Handle All Queues
		AddActorsToCharacterSet();
		RemoveActorsFromCharacterSet();
		DestroyFromQueue();
		RemoveDeadActors();

		TArray<AAI_PawnBase*> Characters = CharacterSet.Array();

		Algo::Sort(Characters, [this](const AAI_PawnBase* Lhs, const AAI_PawnBase* Rhs)
			{
				if (Lhs == nullptr || Rhs == nullptr)
					return false;

				const FVector PlayerLocation = Player->GetActorLocation();
				return FVector::Dist(PlayerLocation, Lhs->GetActorLocation()) < FVector::Dist(PlayerLocation, Rhs->GetActorLocation());
			});

		int CharacterCount = 0;

		for(AAI_PawnBase* Character : Characters)
		{
			if (!IsValid(Character))
				continue;

			if(Character->bIsDead)
			{
				DeadActorQueue.Enqueue(Character);
				continue;
			}

			const FVector PlayerLocation = Player->GetActorLocation();

			if(CharacterCount < NumOfThinkingCharacters && FVector::Dist(PlayerLocation, Character->GetActorLocation()) < TooFarAway)
			{
				if(CharacterCount < NumOfIntelligentCharacters)
				{
					if(Character->GetControllerStatus() != EControllerStatus::Normal && LevelController->Mutex.TryLock())
					{
						// Enable Intelligent Thinking
						if(!LevelController->EnableThinkMap.Contains(Character))
						{
							LevelController->EnableThinkQueue.Enqueue(Character);
							LevelController->EnableThinkMap.Add(Character, EControllerStatus::Normal);
						}
						LevelController->Mutex.Unlock();
					}
				}
				else
				{
					if(Character->GetControllerStatus() != EControllerStatus::Basic && LevelController->Mutex.TryLock())
					{
						// Enable Basic Thinking
						if(!LevelController->EnableThinkMap.Contains(Character))
						{
							LevelController->EnableThinkQueue.Enqueue(Character);
							LevelController->EnableThinkMap.Add(Character, EControllerStatus::Basic);
						}
						LevelController->Mutex.Unlock();
					}
				}

				// If Rendering Is Off Turn It Back On
				if(Character->IsRendering() == false && LevelController->Mutex.TryLock())
				{
					// Resume Rendering
					if(!LevelController->EnableRenderSet.Contains(Character))
					{
						LevelController->EnableRenderQueue.Enqueue(Character);
						LevelController->EnableRenderSet.Add(Character);
					}
					LevelController->Mutex.Unlock();
				}
			}
			else
			{
				// Disable Thinking
				if(Character->GetControllerStatus() != EControllerStatus::Sleep)
				{
					Character->SetEnableThinking(false);

					if (LevelController->Mutex.TryLock())
					{
						if(!LevelController->DisableThinkSet.Contains(Character))
						{
							LevelController->DisableThinkQueue.Enqueue(Character);
							LevelController->DisableThinkSet.Add(Character);
						}
						LevelController->Mutex.Unlock();
					}
				}

				// If AI is too far away from Player, then stop rendering(Anims, etc.)
				if(FVector::Dist(Player->GetActorLocation(), Character->GetActorLocation()) > TooFarAway)
				{
					// Stop Rendering
					if(Character->IsRendering() == true && LevelController->Mutex.TryLock())
					{
						if(!LevelController->DisableRenderSet.Contains(Character))
						{
							LevelController->DisableRenderQueue.Enqueue(Character);
							LevelController->DisableRenderSet.Add(Character);
						}
						LevelController->Mutex.Unlock();
					}
				}
				else if(Character->IsRendering() == false && LevelController->Mutex.TryLock())
				{
					// Resume Rendering if it's off
					if(LevelController->EnableRenderSet.Contains(Character))
					{
						LevelController->EnableRenderQueue.Enqueue(Character);
						LevelController->EnableRenderSet.Add(Character);
					}
					LevelController->Mutex.Unlock();
				}
			}
			CharacterCount++;
		}
	}
	return 0;
}

void FAI_LevelControllerThread::Stop()
{
	if(LevelController->CurrentThread)
	{
		LevelController->CurrentThread->WaitForCompletion();
	}
}

void FAI_LevelControllerThread::AddAIToPool(AAI_PawnBase* Character)
{
	Mutex.Lock();
	AddActorQueue.Enqueue(Character);
	Mutex.Unlock();
}

void FAI_LevelControllerThread::RemoveAIFromPool(AAI_PawnBase* RemoveActor)
{
	RemoveActorQueue.Enqueue(RemoveActor);
}

void FAI_LevelControllerThread::AddActorsToCharacterSet()
{
	while(!AddActorQueue.IsEmpty())
	{
		AAI_PawnBase* OutCharacter;
		AddActorQueue.Dequeue(OutCharacter);

		if (!CharacterSet.Contains(OutCharacter))
		{
			CharacterSet.Emplace(OutCharacter);
		}
	}
}

void FAI_LevelControllerThread::RemoveActorsFromCharacterSet()
{
	while (!RemoveActorQueue.IsEmpty())
	{
		AAI_PawnBase* OutCharacter;
		RemoveActorQueue.Dequeue(OutCharacter);

		if (CharacterSet.Contains(OutCharacter))
		{
			CharacterSet.Remove(OutCharacter);
			DestroyQueue.Enqueue(OutCharacter);
		}
	}
}

void FAI_LevelControllerThread::DestroyFromQueue()
{
	//uint8 Counter = DestroyPerTick;
	while(!DestroyQueue.IsEmpty())
	{
		if(AAI_PawnBase* Pawn; DestroyQueue.Dequeue(Pawn))
		{
			SpawnerController->DespawnRequests.Enqueue(Pawn);
		}
	}
}

void FAI_LevelControllerThread::RemoveDeadActors()
{
	while(!DeadActorQueue.IsEmpty())
	{
		if(AAI_PawnBase* Pawn; DeadActorQueue.Dequeue(Pawn) && CharacterSet.Contains(Pawn))
		{
			CharacterSet.Remove(Pawn);
			LevelController->DestroyQueue.Enqueue(Pawn);
		}
	}
}

// Sets default values
AAI_LevelController::AAI_LevelController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AAI_LevelController::BeginPlay()
{
	Super::BeginPlay();

	APawn* PlayerPtr = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAI_PawnBase::StaticClass(), OutActors);

	TArray<AAI_PawnBase*> Characters;
	for(AActor* Actor : OutActors)
	{
		if (AAI_PawnBase* Character = Cast<AAI_PawnBase>(Actor); IsValid(Character))
			Characters.Add(Character);
	}

	LevelControllerThread = new FAI_LevelControllerThread(PlayerPtr, Characters, NumberOfThinkingCharacters,
		NumberOfIntelligentCharacters, TooFarAwayDistance);

	LevelControllerThread->LevelController = this;

	TArray<AActor*> OutArray;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnerController::StaticClass(), OutArray);

	for (AActor* Actor : OutArray)
	{
		LevelControllerThread->SpawnerController = Cast<ASpawnerController>(Actor);
	}

	CurrentThread = FRunnableThread::Create(LevelControllerThread, TEXT("AI Level Controller Thread"),
		0, EThreadPriority::TPri_BelowNormal);
}

void AAI_LevelController::AddAIToThreadPool(AAI_PawnBase* AICharacter)
{
	if(CurrentThread && LevelControllerThread)
	{
		LevelControllerThread->AddAIToPool(AICharacter);
	}
}

void AAI_LevelController::RemoveAIFromThreadPool(AAI_PawnBase* AICharacter)
{
	if (CurrentThread && LevelControllerThread)
	{
		LevelControllerThread->RemoveAIFromPool(AICharacter);
	}
}

void AAI_LevelController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if(CurrentThread && LevelControllerThread)
	{
		CurrentThread->Suspend(true);
		LevelControllerThread->bStopThread = true;
		CurrentThread->Suspend(false);
		CurrentThread->Kill(false);
		CurrentThread->WaitForCompletion();
		delete LevelControllerThread;
	}

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AAI_LevelController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Mutex.Lock();
	{
		uint8 Counter = ThinkEnablePerTick * FMath::RandBool();
		while(Counter-- > 0 && !EnableThinkQueue.IsEmpty())
		{
			if(AAI_PawnBase* Character; EnableThinkQueue.Dequeue(Character))
			{
				if (TEnumAsByte<EControllerStatus::EType> Status; EnableThinkMap.RemoveAndCopyValue(Character, Status))
				{
					Character->ChangeThinkingStatus(true, Status);

					GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Cyan, "AI: " + Character->GetActorNameOrLabel() + " enabled Thinking");
				}
			}
		}

		Counter = ThinkDisablePerTick;
		while (!DisableThinkQueue.IsEmpty() && Counter --> 0)
		{
			if (AAI_PawnBase* Character; DisableThinkQueue.Dequeue(Character))
			{
				DisableThinkSet.Remove(Character);
				Character->ChangeThinkingStatus(false);

				GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Magenta, "AI: " + Character->GetActorNameOrLabel() + " disabled Thinking");
			}
		}

		Counter = RenderEnablePerTick * FMath::RandBool();
		while (Counter-- > 0 && !EnableRenderQueue.IsEmpty())
		{
			if (AAI_PawnBase* Character; EnableRenderQueue.Dequeue(Character))
			{
				EnableRenderSet.Remove(Character);
				Character->ChangeRenderingStatus(true);

				GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Cyan, "AI: " + Character->GetActorNameOrLabel() + " enabled Rendering");
			}
		}

		Counter = RenderDisablePerTick;
		while (!DisableRenderQueue.IsEmpty() && Counter --> 0)
		{
			if (AAI_PawnBase* Character; DisableRenderQueue.Dequeue(Character))
			{
				DisableRenderSet.Remove(Character);
				Character->ChangeRenderingStatus(false);

				GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Magenta, "AI: " + Character->GetActorNameOrLabel() + " disabled Rendering");
			}
		}

		if (AAI_PawnBase* Pawn; !DestroyQueue.IsEmpty() && DestroyQueue.Dequeue(Pawn))
			Pawn->StartDestroyTimer();
	}
	Mutex.Unlock();
}

