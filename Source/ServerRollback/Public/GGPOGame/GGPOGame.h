#pragma once

#include "include/ggponet.h"
#include "GameState.h"
#include "NonGameState.h"

/*
 * GGPOGame.h --
 *
 * Interface to the internal non-unreal GGPOGame layer.
 *
 */

enum VectorWarInputs {
	INPUT_THRUST            = (1 << 0),
	INPUT_BREAK             = (1 << 1),
	INPUT_ROTATE_LEFT       = (1 << 2),
	INPUT_ROTATE_RIGHT      = (1 << 3),
	INPUT_FIRE              = (1 << 4),
	INPUT_BOMB              = (1 << 5),
 };


int fletcher32_checksum(short* data, size_t len);
uint32 get_time();