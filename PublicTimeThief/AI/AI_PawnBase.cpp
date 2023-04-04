#include "AI_PawnBase.h"
#include "BaseSword.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectTimeThief/AI/Brain/AI_ControllerBase.h"
#include "ProjectTimeThief/AI/Brain/AI_StateManager.h"
#include "ProjectTimeThief/GunBase.h"
#include "ProjectTimeThief/AI/Spawners/AI_SpawnerBase.h"

// Sets default values
AAI_PawnBase::AAI_PawnBase()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Setup Character Components
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule Component"));
	CapsuleComponent->SetCapsuleHalfHeight(88);
	CapsuleComponent->SetCapsuleRadius(36);
	RootComponent = CapsuleComponent;

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Character Mesh"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow Component"));
	ArrowComponent->SetupAttachment(RootComponent);

	// Set Up Movement Component
	MovementComponent = CreateDefaultSubobject<UNonPlayerCharacterMovement>(TEXT("Pawn Movement Component"));

	StateManager = CreateDefaultSubobject<UAI_StateManager>(TEXT("State Manager Component"));
	StateManager->SetComponentTickEnabled(false);
	StateManager->SetComponentTickInterval(0.1f);
}

// Called when the game starts or when spawned
void AAI_PawnBase::BeginPlay()
{
	StateManager->Character = this;

	Super::BeginPlay();

	if (AIController)
		AIController->GetBlackboardComponent()->SetValueAsFloat(FName("Time Since Destroy"), -1.f);

	// Setup Notifier Component
	{
		Notifier = Cast<USphereComponent>(GetDefaultSubobjectByName(TEXT("Notifier Component")));

		if (IsValid(Notifier))
		{
			Notifier->OnComponentBeginOverlap.AddDynamic(this, &AAI_PawnBase::OnNotifierBeginOverlap);
			Notifier->OnComponentEndOverlap.AddDynamic(this, &AAI_PawnBase::OnNotifierEndOverlap);
		}
	}

	// Make sure Sword or Gun pointer is set
	if(!Sword || !Gun)
	{
		TArray<AActor*> OutChildren;
		GetAllChildActors(OutChildren);

		for(AActor* Actor : OutChildren)
		{
			if(Actor->IsA(ABaseSword::StaticClass()))
			{
				Sword = Cast<ABaseSword>(Actor);
			}
			if(Actor->IsA(AGunBase::StaticClass()))
			{
				Gun = Cast<AGunBase>(Actor);
			}
		}
	}

	AnimInstance = MeshComponent->GetAnimInstance();
}

// Called every frame
void AAI_PawnBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (AIController)
	{
		if (UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent(); IsValid(Blackboard) && bIsThinking)
		{
			Blackboard->SetValueAsFloat(HealthKey, Health);
			Blackboard->SetValueAsFloat(FearKey, Fear);
			Blackboard->SetValueAsFloat(ConfidenceKey, Confidence);
			Blackboard->SetValueAsFloat(SusKey, Suspicion);

			// If the AI was recently in the destroy state
			if (TimeSinceDestroy >= 0.f)
			{
				TimeSinceDestroy += DeltaTime;
				// Reset TimeSinceDestroy after a set amount of time (TimeToForgetDestroy)
				if (TimeSinceDestroy >= TimeToForgetDestroy)
				{
					ResetTimeSinceDestroy();
				}
				Blackboard->SetValueAsFloat(FName("Time Since Destroy"), TimeSinceDestroy);
			}
		}
	}
}

// Called to bind functionality to input
void AAI_PawnBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

UPawnMovementComponent* AAI_PawnBase::GetMovementComponent() const
{
	return MovementComponent;
}

void AAI_PawnBase::RespondTo(FVector RespondToLocation)
{
	// Set Values
	RespondLocation = RespondToLocation;

	GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Magenta, GetActorNameOrLabel() + " Responding...");

	switch (StateManager->GetCurrentState())
	{
	case AI_State::Patrol:
		// Respond & Raise Suspicion so that there is not an immediate exit of Search State
		Suspicion = 80.f;

		// Switch State to Search and switch substate to Responding
		{
			StateManager->SwitchStateNextTick(AI_State::Search);

			if (UAI_StateBase* SearchState = StateManager->GetSpecificState(AI_State::Search); IsValid(SearchState))
			{
				SearchState->SwitchSubstate(ESubstate::Responding);
			}
		}
		break;

	case AI_State::Search:
		// Respond if not already tracking a Known Hostile
		if (!GetAIController()->IsThereKnownHostilesVisible())
		{
			// Switch State to Search and switch substate to Responding
			StateManager->SwitchStateNextTick(AI_State::Search);
			if (UAI_StateBase* SearchState = StateManager->GetSpecificState(AI_State::Search); IsValid(SearchState))
			{
				SearchState->SwitchSubstate(ESubstate::Responding);
			}
		}
		break;

	case AI_State::Destroy:
		// Ignore
		break;

	default:
		// Ignore
		break;
	}

}

void AAI_PawnBase::OnNotifierBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AAI_PawnBase* PawnBase = Cast<AAI_PawnBase>(OtherActor); OtherActor != this && IsValid(PawnBase))
	{
		NotifiedActors.Emplace(PawnBase);
	}
}

void AAI_PawnBase::OnNotifierEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (const AAI_PawnBase* PawnBase = Cast<AAI_PawnBase>(OtherActor); IsValid(PawnBase))
	{
		NotifiedActors.Remove(PawnBase);
	}
}

void AAI_PawnBase::SetEnableThinking(const bool bSet, const TEnumAsByte<EControllerStatus::EType> ControllerStatus)
{
	bChangeThinkingStatusOnTick = true;
	bSetThinkingStatusTo = bSet;
	ControllerStatusToSet = ControllerStatus;
}

void AAI_PawnBase::SetEnableRendering(const bool bSet)
{
	bChangeRenderingStatusOnTick = true;
	bSetRenderingStatusTo = bSet;
}

void AAI_PawnBase::ChangeThinkingStatus(bool bSet, const TEnumAsByte<EControllerStatus::EType> ControllerStatus)
{
	bIsThinking = bSet;
	StateManager->SetComponentTickEnabled(bSet);
	MovementComponent->SetComponentTickEnabled(bSet);

	if (AIController)
	{
		switch (ControllerStatus)
		{
		case EControllerStatus::Sleep:
			Notifier->SetComponentTickEnabled(false);
			AIController->SetEnableThinking(false, ControllerStatus);
			break;
		case EControllerStatus::Basic:
			Notifier->SetComponentTickEnabled(false);
			AIController->SetEnableThinking(true, ControllerStatus);
			MovementComponent->SetComponentTickInterval(0.05f);
			break;
		case EControllerStatus::Normal:
			Notifier->SetComponentTickEnabled(true);
			AIController->SetEnableThinking(true, ControllerStatus);
			MovementComponent->SetComponentTickInterval(0.03f);
			break;
		default:
			break;
		}
	}
}

// Pause or Resume Render Status
void AAI_PawnBase::ChangeRenderingStatus(bool bSet)
{
	bIsRendering = bSet;

	TArray<UActorComponent*> OutComponents;
	GetComponents(USkeletalMeshComponent::StaticClass(), OutComponents);

	for (UActorComponent* Component : OutComponents)
	{
		USkeletalMeshComponent* SKMeshComponent = Cast<USkeletalMeshComponent>(Component);

		// Set to True if Render Status(bSet) is False vis versa
		SKMeshComponent->bPauseAnims = !bSet; 

		if(bSet)
		{
			SKMeshComponent->SetCollisionProfileName(TEXT("CharacterMesh"));
		}
		else
		{
			SKMeshComponent->SetCollisionProfileName(TEXT("NoCollision"));
		}
	}
}

float AAI_PawnBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
{
	if(Health <= 0)
		return 0;

	float DamageTaken = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	FVector Direction;
	FHitResult Hit;
	DamageEvent.GetBestHitInfo(this, DamageCauser, Hit, Direction);

	if (!AIController)
		return DamageTaken;

	// Check if the attack is a stealth attack
	if (!AIController->IsThereKnownHostilesVisible() && Suspicion < 80.f)
		DamageAmount *= 2.5f;

	if (DamageAmount >= Health)
	{
		DamageTaken = Health;
		Health = 0;

		// Handle Death
		BeginToDie(-Direction, Hit.Location);

		int32 SoundIndex = FMath::RandRange(0, DeathSounds.Num() - 1);
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DeathSounds[SoundIndex], GetActorLocation(),
			GetActorRotation(), SoundClass->Properties.Volume, 1, 0, AttenuationClass);
	}
	else
	{
		Health -= DamageAmount;

		// Raise suspicion almost 100 points, but if suspicion is zero the AI will not enter destroy
		Suspicion += 99.99999f;

		// Respond to a location in the direction of the shot
		RespondTo(GetActorLocation() - ((GetActorLocation() - DamageCauser->GetActorLocation()) * 0.8f));

		// Play a Damage Sound
		int32 SoundIndex = FMath::RandRange(0, DamageSounds.Num() - 1);
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DamageSounds[SoundIndex], GetActorLocation(),
			GetActorRotation(), SoundClass->Properties.Volume, 1, 0, AttenuationClass);
	}

	return DamageTaken;
}

void AAI_PawnBase::BeginToDie(FVector DirectionToDie, FVector HitLocation)
{
	if(!bIsDead)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, "Direction to die: " + DirectionToDie.ToCompactString());

		// Un-possess and get rid of of controller
		if(AIController)
		{
			AIController->UnPossess();
			AIController->Destroy();
		}

		// Destroy Components except Mesh
		TArray<UActorComponent*> PawnComponents;
		GetComponents(PawnComponents);

		for (UActorComponent* Component : PawnComponents)
		{
			if (Component->GetClass() != USkeletalMeshComponent::StaticClass())
				Component->DestroyComponent(true);
		}


		// Ragdoll Mesh and Set Mesh to Collide with everything
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComponent->SetCollisionProfileName(TEXT("Ragdoll"));
		MeshComponent->SetSimulatePhysics(true);
		// MeshComponent->AddImpulseToAllBodiesBelow(DirectionToDie * 700.f);
		MeshComponent->bPauseAnims = true;

		// Start Event to Disable Ragdoll and lock the position
		GetWorld()->GetTimerManager().SetTimer(StopRagdollTimerHandle, this, &AAI_PawnBase::StopRagdoll, 2.0f, false);

		bIsDead = true;


		// If connected to a spawner, handle death within the spawner;
		if(Spawner != nullptr)
		{
			if (!bIsShepherd)
			{
				Spawner->KillSheepByPtr(this);
			}
			else
			{
				Spawner->FindNewShepherd();
			}
		}
	}

}

void AAI_PawnBase::StopRagdoll()
{
	if(MeshComponent && GetVelocity().Size() < 50.f)
	{
		MeshComponent->PutAllRigidBodiesToSleep();
		MeshComponent->SetComponentTickEnabled(false);

		SetActorTickEnabled(false);

		GetWorld()->GetTimerManager().ClearTimer(StopRagdollTimerHandle);
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(StopRagdollTimerHandle, this, &AAI_PawnBase::StopRagdoll, 2.0f, false);
	}
}

void AAI_PawnBase::StartDestroyTimer()
{
	GetWorld()->GetTimerManager().SetTimer(DestroyAfterDeathTimerHandle, this, &AAI_PawnBase::DestroyAfterDeath, 10.0f, false);
}

void AAI_PawnBase::DestroyAfterDeath()
{
	Destroy();
}

void AAI_PawnBase::Attack()
{
	if(Sword)
	{
		Sword->SetAttacking(true);

		const int32 MontageCount = AttackMontages.Num() - 1;
		const int32 MontageSelected = FMath::RandRange(0, MontageCount);
		UAnimMontage* AttackMontage = AttackMontages[MontageSelected];

		AnimInstance->Montage_Play(AttackMontage);

		int32 SoundIndex = FMath::RandRange(0, SwordSoundArray.Num() - 1);
		UGameplayStatics::PlaySound2D(GetWorld(), SwordSoundArray[SoundIndex], SoundClass->Properties.Volume, FMath::RandRange(0.95f, 1.05f));
	}
	else if(Gun)
	{
		// Need to be aiming near a hostile
		if(!TargetHostile)
		{
			return;
		}

		FVector Aim = TargetHostile->GetActorLocation();

		const int32 RandX = FMath::RandRange(-WeaponAccuracy, WeaponAccuracy);
		const int32 RandY = FMath::RandRange(-WeaponAccuracy, WeaponAccuracy);
		const int32 RandZ = FMath::RandRange(-WeaponAccuracy, WeaponAccuracy);
		const FVector AccuracyEffect = FVector(RandX, RandY, RandZ);

		Aim = Aim + AccuracyEffect;

		// Fire Gun
		if(Gun->Fire())
		{
			// Play Fire Animation
			if(ShootMontage)
				AnimInstance->Montage_Play(ShootMontage);

			FHitResult Hit;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);
			Params.AddIgnoredActor(Gun);

			const FVector ShootingVector = GetActorLocation() + (Aim - GetActorLocation()) * 2;
			const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), ShootingVector, ECC_Pawn, Params);
			DrawDebugLine(GetWorld(), GetActorLocation(), ShootingVector, bHit ? FColor::Red : FColor::Blue , false, 5);

			if(bHit)
			{
				DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 10, FColor::Red, false, 5);
				if(APawn* HitPawn = Cast<APawn>(Hit.GetActor()))
				{
					FPointDamageEvent DamageEvent(16.f, Hit, GetActorRotation().Vector(), nullptr);
					HitPawn->TakeDamage(34, DamageEvent, GetController(), this);
				}
			}
		}
		else if(Gun->GetMagazineAmmo() <= 0 && Gun->Reload())
		{
			// Play Reload Animation
			if (ReloadMontage)
				AnimInstance->Montage_Play(ReloadMontage);

		}
	}
}

void AAI_PawnBase::Defend()
{
	if(Sword)
	{
		// Block
	}
}

//==================================

FAI_PawnNotifier::FAI_PawnNotifier(TSet<AAI_PawnBase*> Characters, FVector ActorLocation)
{
	CharactersToNotify = Characters;
	NotifyLocation = ActorLocation;
}

FAI_PawnNotifier::~FAI_PawnNotifier()
{
	UE_LOG(LogTemp, Warning, TEXT("Notify Task Finished!!!"));
}

void FAI_PawnNotifier::DoWork() const
{
	for (AAI_PawnBase* CharacterToNotify : CharactersToNotify)
	{
		CharacterToNotify->RespondTo(NotifyLocation);
	}
}

