// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BulletPlayerController.generated.h"

USTRUCT(BlueprintType)
struct FBulletInput
{
	GENERATED_USTRUCT_BODY()
	//Movement vector for movement + jump
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector MoveInputVector;
	//Camera View rotator
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FRotator LookInputRotator;
	//Player pressing Fire button
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bInputFire;

	FBulletInput()	{	MoveInputVector = FVector();	LookInputRotator = FRotator();	bInputFire = false;	}
	FBulletInput(FVector MoveInput_, FRotator LookInput_, bool FireInput_ = false)	{	MoveInputVector = MoveInput_;	LookInputRotator = LookInput_;	bInputFire = FireInput_;	}
};

UCLASS()
class SERVERROLLBACK_API ABulletPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	FBulletInput CurrentInput = { FVector(), FRotator(), false };
	
	FBulletInput GetUEBulletInput() const;
};
