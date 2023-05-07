// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BulletPlayerController.generated.h"


UCLASS()
class SERVERROLLBACK_API ABulletPlayerController : public APlayerController
{
	GENERATED_BODY()	
public:
	int32 GetBulletInput();
};
