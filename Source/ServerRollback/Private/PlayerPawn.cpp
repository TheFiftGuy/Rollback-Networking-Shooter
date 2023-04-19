// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerPawn.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "Kismet/GameplayStatics.h"

//Bullet Phys
#include "PhysicsWorldActor.h"
#include "BulletMain.h"
#include "BulletHelpers.h"

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
	
	BulletWorldActor = CastChecked<APhysicsWorldActor>(UGameplayStatics::GetActorOfClass(GetWorld(), APhysicsWorldActor::StaticClass()));
	PlayerBody = BulletWorldActor->AddPhysicsPlayer(this);
}

// Called every frame
void APlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Rotate character's Pitch (left right)
	FRotator InputRot = GetControlRotation();
	InputRot.Pitch = 0; InputRot.Roll = 0;
	btQuaternion BodyRot = BulletHelpers::ToBt(InputRot);
	PlayerBody->getWorldTransform().setRotation(BodyRot);//unsure if this way or motionState() way is deterministic.

	//move player
	FVector InputVec = ConsumeMovementInputVector();
	InputVec.Normalize(1);
	btVector3 BodyVel =	BulletHelpers::ToBtDir(InputVec * MaxWalkSpeed, false);

	IsGrounded();
	//if jump input
	if(InputVec.Z > 0)
	{
		//apply vertical impulse
		BodyVel.setZ(BulletHelpers::ToBtDir(FVector(0.f, 0.f, JumpForce)).getZ());
		bInAir = true;
		PlayerBody->setGravity(BulletWorldActor->GetBtWorld()->getGravity());
		//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("Player Jump Force applied"));
	}
	//if grounded but not jumping. (this prevents sliding down small inclines)
	else if(!bInAir && PlayerBody->getLinearVelocity().getZ() <= 0)
	{
		BodyVel.setZ(0);
		PlayerBody->setGravity(btVector3());
		//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, TEXT("Standing"));
	}
	//if mid-air
	else
	{
		//keep current momentum
		PlayerBody->setGravity(BulletWorldActor->GetBtWorld()->getGravity());
		BodyVel.setZ(PlayerBody->getLinearVelocity().getZ());
		//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("Momentum"));
	}
	
	PlayerBody->setLinearVelocity(BodyVel);
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

		//jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerPawn::Jump);

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

void APlayerPawn::Jump(const FInputActionValue& Value)
{
	//if standing on something
	if(IsGrounded())
	{
		//input is a bool
		const bool bJumpInput = Value.Get<bool>();
		if (Controller != nullptr)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("Player Jumped"));
			AddMovementInput(GetActorUpVector(), bJumpInput);
			bInAir = true;
		}
	}
	bInAir = true;
}

bool APlayerPawn::IsGrounded()
{
	btTransform btPlayerTransform;
	PlayerBody->getMotionState()->getWorldTransform(btPlayerTransform);
	
	btVector3 From = btPlayerTransform.getOrigin();
	btVector3 Offset = btVector3(0.f, 0.f, - (0.96 + 0.25));
	btVector3 To = From + Offset;
	
	btCollisionWorld::ClosestRayResultCallback rayCallback(From, To);
	BulletWorldActor->GetBtWorld()->rayTest(btPlayerTransform.getOrigin(), To, rayCallback);
	
	//if standing on something
	if(rayCallback.hasHit())
	{
		//UE_LOG(LogTemp, Warning, TEXT("Jump raycast hit: %hs"), rayCallback.m_collisionObject->getCollisionShape()->getName());
		bInAir = false;
		return true;
	}
	//if mid-air
	bInAir = true;
	return false;
}

