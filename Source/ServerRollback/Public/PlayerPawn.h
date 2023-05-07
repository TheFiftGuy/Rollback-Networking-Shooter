// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PlayerPawn.generated.h"


class UCapsuleComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class APhysicsWorldActor;
class btRigidBody;

UCLASS()
class SERVERROLLBACK_API APlayerPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	APlayerPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	//virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	const UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }

	UPROPERTY(Category=Character, EditDefaultsOnly)
	int MaxWalkSpeed = 10;
	
	UPROPERTY(Category=Character, EditDefaultsOnly)
	int JumpForce = 750;
	
private:

	//Components
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CapsuleComponent;
	
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> FPSMesh;
	
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> GunMesh;
	
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera;

public:
	/*//Input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;
	
	/** Jump Input Action #1#
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action #1#
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action #1#
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Fire Input Action #1#
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* FireAction;*/

private:
	//Bullet Stuff
	APhysicsWorldActor* BulletWorldActor;
	
	btRigidBody* PlayerBody;
	
protected:
	/** Called for movement input */
	/*
	void Move(const FInputActionValue& Value);

	/** Called for looking input #1#
	void Look(const FInputActionValue& Value);

	/** Called for jumping input #1#
	void Jump(const FInputActionValue& Value);
	
	/** Called for Fire/Shoot input #1#
	void Fire();

	void StopMoving();
	void StopJumping();
	void StopFiring();
	*/

	bool IsGrounded();
};
