// Fill out your copyright notice in the Description page of Project Settings.


#include "BulletPlayerController.h"

#include "EnhancedInputComponent.h"
#include "PlayerPawn.h"

FBulletInput ABulletPlayerController::GetUEBulletInput() const
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Emerald, FString::Printf(TEXT("BulletController: Move = %s  || Look = %s || Fire = %d"), *CurrentInput.MoveInputVector.ToString(), *CurrentInput.LookInputRotator.ToString(), CurrentInput.bInputFire));
	return  CurrentInput;
}
