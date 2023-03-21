// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerPawn.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"

#include "EnhancedInputComponent.h"
#include "PhysicsWorldActor.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
APlayerPawn::APlayerPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	// Set size for collision capsule
	CapsuleComponent->InitCapsuleSize(55.f, 96.0f);
	SetRootComponent(CapsuleComponent);
	
	// Create a CameraComponent	
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPCamera"));
	Camera->SetupAttachment(CapsuleComponent);
	Camera->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	Camera->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	FPSMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FPMesh"));
	//FPSMesh->SetOnlyOwnerSee(true);
	FPSMesh->SetupAttachment(Camera);
	
	// FPSMesh->bCastDynamicShadow = false;
	// FPSMesh->CastShadow = false;

	FPSMesh->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	GunMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Gun"));
	

}

// Called when the game starts or when spawned
void APlayerPawn::BeginPlay()
{
	Super::BeginPlay();
	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	GunMesh->AttachToComponent(FPSMesh, AttachmentRules, FName(TEXT("GripPoint")));
}

// Called every frame
void APlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//If there is movement to do
	if(!GetPendingMovementInputVector().IsZero())
	{
		AActor* btWorldActor = UGameplayStatics::GetActorOfClass(GetWorld(), APhysicsWorldActor::StaticClass());
		if (APhysicsWorldActor* btWorld = CastChecked<APhysicsWorldActor>(btWorldActor))
		{
			btWorld->UpdatePlayerPhysics(this);

		}
	}
	

}

// Called to bind functionality to input
void APlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//Jumping
		// EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		// EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerPawn::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerPawn::Look);
	}
}

void APlayerPawn::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void APlayerPawn::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

