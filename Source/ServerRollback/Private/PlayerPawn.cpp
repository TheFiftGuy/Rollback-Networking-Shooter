// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerPawn.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputSubsystems.h"


//Bullet Phys
#include "PhysicsWorldActor.h"
#include "BulletMain.h"
#include "BulletHelpers.h"
#include "BulletPlayerController.h"

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
	
	if (auto* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if(DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
	
	BulletWorldActor = CastChecked<APhysicsWorldActor>(UGameplayStatics::GetActorOfClass(GetWorld(), APhysicsWorldActor::StaticClass()));
	PlayerBody = BulletWorldActor->AddPhysicsPlayer(this);
}

// Called every frame
void APlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FBulletInput Input;
	if(ABulletPlayerController* BulletController = Cast<ABulletPlayerController>(Controller))
	{
		Input = BulletController->GetUEBulletInput();
	}
	
	//Rotate character's Pitch (left right)
	FRotator InputRot = Input.LookInputRotator;
	InputRot.Pitch = 0; InputRot.Roll = 0;
	btQuaternion BodyRot = BulletHelpers::ToBt(InputRot);
	PlayerBody->getWorldTransform().setRotation(BodyRot);//unsure if this way or motionState() way is deterministic.

	//move player
	FVector InputVec = Input.MoveInputVector;
	InputVec.Normalize(1);
	btVector3 BodyVel =	BulletHelpers::ToBtDir(InputVec * MaxWalkSpeed, false);

	
	//if jump input
	bool bGrounded = IsGrounded();
	if(InputVec.Z > 0 && bGrounded)
	{
		//apply vertical velocity
		BodyVel.setZ(BulletHelpers::ToBtDir(FVector(0.f, 0.f, JumpForce)).getZ());
		//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("Player Jump Force applied"));
	}
	else
	{
		BodyVel.setZ(PlayerBody->getLinearVelocity().getZ());
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
		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerPawn::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &APlayerPawn::StopMoving);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerPawn::Look);

		//jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &APlayerPawn::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlayerPawn::StopJumping);


		//firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &APlayerPawn::Fire);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &APlayerPawn::StopFiring);

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
		if(ABulletPlayerController* BulletController = CastChecked<ABulletPlayerController>(Controller))
		{
			BulletController->CurrentInput.MoveInputVector = ConsumeMovementInputVector();
		}
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
		if(ABulletPlayerController* BulletController = CastChecked<ABulletPlayerController>(Controller))
		{
			BulletController->CurrentInput.LookInputRotator = GetControlRotation();
		}
	}
}

void APlayerPawn::Jump(const FInputActionValue& Value)
{
	//we always receive jump input, even when player cannot jump (aka mid-air, or holding jump)
	const bool bJumpInput = Value.Get<bool>();
	if (Controller != nullptr)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("Player Jumped"));
		AddMovementInput(GetActorUpVector(), bJumpInput);

		if(ABulletPlayerController* BulletController = CastChecked<ABulletPlayerController>(Controller))
		{
			BulletController->CurrentInput.MoveInputVector.Z = 1;
		}
	}

}

void APlayerPawn::Fire()
{
	//Temp
	if(ABulletPlayerController* BulletController = CastChecked<ABulletPlayerController>(Controller))
	{
		BulletController->CurrentInput.bInputFire = true;
		//BulletController->GetUEBulletInput();
	}
	//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("Player Fired"));
	btTransform btPlayerTransform;
	PlayerBody->getMotionState()->getWorldTransform(btPlayerTransform);
	//btVector3 From = btPlayerTransform.getOrigin();
	btVector3 From = btPlayerTransform.getOrigin();
	
	btVector3 Offset = BulletHelpers::ToBtPos(Camera->GetRelativeLocation(), BulletWorldActor->GetActorLocation());
	From += Offset;
	
	btVector3 CamDir = BulletHelpers::ToBtDir(Camera->GetForwardVector()) * 10000.f; //prob bigger than necessary 
	btVector3 To = From + CamDir;
	
	btCollisionWorld::ClosestRayResultCallback rayCallback(From, To);
	BulletWorldActor->GetBtWorld()->rayTest(From, To, rayCallback);

	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("Player Fired"));
	DrawDebugLine(GetWorld(), BulletHelpers::ToUEPos(From,BulletWorldActor->GetActorLocation()), BulletHelpers::ToUEPos(To,BulletWorldActor->GetActorLocation()),FColor::Green, false, 1.f, 0, 1);
	
	if(rayCallback.hasHit())
	{
		//I'm sorry but it has to be cast this way. Unreal doesn't like turning dumb void* into Unreal AActor*.
		APlayerPawn* HitPlayer = (APlayerPawn*)rayCallback.m_collisionObject->getUserPointer();
		
		if (HitPlayer->IsA(APlayerPawn::StaticClass()))
		{
			//Player has successfully shot another player
			GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, FString::Printf(TEXT("Hit: %s"), *HitPlayer->GetName()));
		}
	}
}

void APlayerPawn::StopMoving()
{
	if(ABulletPlayerController* BulletController = CastChecked<ABulletPlayerController>(Controller))
	{
		BulletController->CurrentInput.MoveInputVector.X = 0;
		BulletController->CurrentInput.MoveInputVector.Y = 0;
		//BulletController->GetUEBulletInput();

	}
}

void APlayerPawn::StopJumping()
{
	if(ABulletPlayerController* BulletController = CastChecked<ABulletPlayerController>(Controller))
	{
		BulletController->CurrentInput.MoveInputVector.Z = 0;
		//BulletController->GetUEBulletInput();

	}
}

void APlayerPawn::StopFiring()
{
	if(ABulletPlayerController* BulletController = CastChecked<ABulletPlayerController>(Controller))
	{
		BulletController->CurrentInput.bInputFire = false;
		//BulletController->GetUEBulletInput();
	}
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

		return true;
	}
	return false;
}

