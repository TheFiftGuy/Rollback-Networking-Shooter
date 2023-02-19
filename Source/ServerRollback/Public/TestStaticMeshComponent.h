// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "TestStaticMeshComponent.generated.h"

/**
 * 
 */
UCLASS()
class SERVERROLLBACK_API UTestStaticMeshComponent : public UStaticMeshComponent
{
	GENERATED_BODY()
public:
	UTestStaticMeshComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
	float ElapsedTime = 0.0f;
	int32 ElapsedTicks = 0;
};
