// Fill out your copyright notice in the Description page of Project Settings.


#include "BulletPlayerController.h"

FBulletInput ABulletPlayerController::GetUEBulletInput()
{
	FBulletInput Input = { FVector(), FRotator(), false };

	Input.MoveInputVector = GetPawn()->ConsumeMovementInputVector();
	Input.LookInputRotator = FRotator(RotationInput.Pitch, RotationInput.Yaw, 0);
	Input.bInputFire = bInputtingFire;

	return  Input;
}
