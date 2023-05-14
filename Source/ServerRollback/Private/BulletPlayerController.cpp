// Fill out your copyright notice in the Description page of Project Settings.

#include "BulletPlayerController.h"
#include "GGPOGame/GGPOGame.h"

int32 ABulletPlayerController::GetBulletInput()
{
	//Get standard keys
	/*static const struct {
		FKey     key;
		int32    input;
	} inputtable[] = {
		{ EKeys::W,       INPUT_FORWARDS },
		{ EKeys::S,     INPUT_BACKWARDS },
		{ EKeys::A,     INPUT_LEFT },
		{ EKeys::D,    INPUT_RIGHT },
		{ EKeys::LeftMouseButton,        INPUT_FIRE },
		{ EKeys::SpaceBar,        INPUT_JUMP },
	 };
	InputData inputs;

	for (int i = 0; i < sizeof(inputtable) / sizeof(inputtable[0]); i++) {
		if (IsInputKeyDown(inputtable[i].key)) {
			inputs |= inputtable[i].input;
		}
	}*/
	
	int32 InputMask = 0;
	
	//Get standard keys
	if(IsInputKeyDown(EKeys::W))
	{
		InputMask |= INPUT_FORWARDS;
	}
	if(IsInputKeyDown(EKeys::S))
	{
		InputMask |= INPUT_BACKWARDS;
	}
	if (IsInputKeyDown(EKeys::A)) {
		InputMask |= INPUT_LEFT;
	}
	if (IsInputKeyDown(EKeys::D)) {
		InputMask |= INPUT_RIGHT;
	}
	if (IsInputKeyDown(EKeys::LeftMouseButton)) {
		InputMask |= INPUT_FIRE;
	}
	if (IsInputKeyDown(EKeys::SpaceBar)) {
		InputMask |= INPUT_JUMP;
	}

	//get mouse
	FVector2D MouseDelta = FVector2D();
	GetInputMouseDelta(MouseDelta.X, MouseDelta.Y);
	//UE_LOG(LogTemp, Log, TEXT("UE X= %f UE Y= %f"), MouseDelta.X, MouseDelta.Y);

	//turns decimal float range [-1.0f , 1.0f] to int [-4096, 4096]
	int32 MouseDeltaX = static_cast<int32>(MouseDelta.X/100 * 4096.0f); //used to be * 8192.0f
	int32 MouseDeltaY = static_cast<int32>(MouseDelta.Y/100 * 4096.0f);

	// Store the mouse deltas in the bitmask starting at bit 6
	InputMask |= ((MouseDeltaX & 0x1FFF) << 6);
	InputMask |= ((MouseDeltaY & 0x1FFF) << 19);
	
	return InputMask;
}
