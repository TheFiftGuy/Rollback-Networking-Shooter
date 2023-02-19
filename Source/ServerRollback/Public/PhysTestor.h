// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PhysTestor.generated.h"

UCLASS()
class SERVERROLLBACK_API APhysTestor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APhysTestor();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
private:
	float ElapsedTime = 0.0f;
	int32 ElapsedTicks = 0;

	USceneComponent* SceneComponent;
	class UTestStaticMeshComponent* TestComponent;

};
