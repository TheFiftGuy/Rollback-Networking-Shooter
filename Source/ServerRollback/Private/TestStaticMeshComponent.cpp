// Fill out your copyright notice in the Description page of Project Settings.


#include "TestStaticMeshComponent.h"

UTestStaticMeshComponent::UTestStaticMeshComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	
	
}

void UTestStaticMeshComponent::BeginPlay()
{
	Super::BeginPlay();
	//unreal Phys
	//SetSimulatePhysics(true);
	//SetEnableGravity(true);
}

void UTestStaticMeshComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	/*
	ElapsedTime += DeltaTime;
	ElapsedTicks++;
	UE_LOG(LogTemp, Warning, TEXT("Comp: Pos %s, Time %f, and TickNum %d"), *GetComponentTransform().GetLocation().ToString(), ElapsedTime, ElapsedTicks);
	*/

}
