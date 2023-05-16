#pragma once

#include "include/ggponet.h"
#include "GameState.h"
#include "NonGameState.h"

//Interface class/header holder for the non UE gamestate stuff

//Enable singleplayer sync test using this:
//#define SYNC_TEST = true;

enum BulletInputs {
	INPUT_FORWARDS   = (1 << 0),
	INPUT_BACKWARDS  = (1 << 1),
	INPUT_LEFT       = (1 << 2),
	INPUT_RIGHT      = (1 << 3),
	INPUT_FIRE       = (1 << 4),
	INPUT_JUMP       = (1 << 5),
};
#define FRAME_DELAY        2

int fletcher32_checksum(short* data, size_t len);
uint32 get_time();