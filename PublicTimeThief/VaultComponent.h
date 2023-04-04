// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Thief.h"
#include "VaultComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BROWSERTIMETHIEFC_API UVaultComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UVaultComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Returns True If Character Can Vault
	bool CharacterCanVault(FVector& OutVaultToVector);

	// Returns True If Projected Path does not hit any obstructing geometry
	bool ProjectVaultPath(UWorld* World, FVector& CurrentLocation, FVector& VaultEnd, float CapsuleHalfHeight, float CapsuleRadius, FQuat
	                      Quaternion, FCollisionQueryParams Params) const;

private:
	UPROPERTY(EditAnywhere)
	int32 NumberOfVaultPoints = 4;

	UPROPERTY(EditAnywhere)
	float MaxVaultHeight = 400;

	UPROPERTY(EditAnywhere)
	float DistanceFromPlayer = 75;

	UPROPERTY(VisibleAnywhere)
	AThief* Thief;
};

