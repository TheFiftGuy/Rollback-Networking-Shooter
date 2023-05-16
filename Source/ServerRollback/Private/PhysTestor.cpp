// Fill out your copyright notice in the Description page of Project Settings.


#include "PhysTestor.h"

#include "TestStaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values
APhysTestor::APhysTestor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("StaticMesh'/Engine/BasicShapes/Sphere'"));

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	//SetRootComponent(SceneComponent);
	
	TestComponent = CreateDefaultSubobject <UTestStaticMeshComponent>(TEXT("TestStaticMeshComponent"));
	TestComponent->SetStaticMesh(SphereMesh.Object);
	//const FAttachmentTransformRules Attachment = FAttachmentTransformRules( EAttachmentRule::KeepRelative, true );

	
	//TestComponent->AttachToComponent(SceneComponent, FAttachmentTransformRules(EAttachmentRule::KeepRelative, true));
	SetRootComponent(TestComponent);

	AddTickPrerequisiteComponent(TestComponent);
}

// Called when the game starts or when spawned
void APhysTestor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APhysTestor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	/*ElapsedTime += DeltaTime;
	ElapsedTicks++;
	UE_LOG(LogTemp, Warning, TEXT("Acto: Pos %s, Time %f, and TickNum %d"), *GetActorTransform().GetLocation().ToString(), ElapsedTime, ElapsedTicks);

	if(ElapsedTicks >= 300)
	{
		UKismetSystemLibrary::QuitGame(GetWorld(), 0, EQuitPreference::Quit, false);
	}*/

}

