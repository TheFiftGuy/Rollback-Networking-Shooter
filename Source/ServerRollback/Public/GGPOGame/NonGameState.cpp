#include "nongamestate.h"

// UE4: allow Windows platform types to avoid naming collisions
//  this must be undone at the bottom of this file
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/prewindowsapi.h"

#include <timeapi.h>

void NonGameState::SetConnectState(GGPOPlayerHandle handle, EPlayerConnectState state) {
	for (int i = 0; i < num_players; i++) {
		if (players[i].handle == handle) {
			players[i].connect_progress = 0;
			players[i].state = state;
			break;
		}
	}
}

void NonGameState::SetDisconnectTimeout(GGPOPlayerHandle handle, int when, int timeout) {
	for (int i = 0; i < num_players; i++) {
		if (players[i].handle == handle) {
			players[i].disconnect_start = when;
			players[i].disconnect_timeout = timeout;
			players[i].state = EPlayerConnectState::Disconnecting;
			break;
		}
	}
}

float NonGameState::GetDisconnectTime(int32 index) const
{
	FPlayerConnectionInfo info = players[index];
	return ((timeGetTime() - info.disconnect_start) * 1.0f) / info.disconnect_timeout;
}

void NonGameState::SetConnectState(EPlayerConnectState state) {
	for (int i = 0; i < num_players; i++) {
		players[i].state = state;
	}
}

void NonGameState::UpdateConnectProgress(GGPOPlayerHandle handle, int progress) {
	for (int i = 0; i < num_players; i++) {
		if (players[i].handle == handle) {
			players[i].connect_progress = progress;
			break;
		}
	}
}

// UE4: disallow windows platform types
//  this was enabled at the top of the file
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"
