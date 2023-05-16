#pragma once
#include "CoreMinimal.h"
#include "include/ggponet.h"
#include "nongamestate.generated.h"

#define MAX_PLAYERS     64

//entirely sourced from https://github.com/BwdYeti/VectorWarUE4
//largely unsused but kept to keep GGPO working

/*
 * nongamestate.h --
 *
 * These are other pieces of information not related to the state
 * of the game which are useful to carry around.  They are not
 * included in the GameState class because they specifically
 * should not be rolled back.
 */

UENUM(BlueprintType)
enum class EPlayerConnectState : uint8 {
	Connecting = 0   UMETA(DisplayName = "Connecting"),
	Synchronizing    UMETA(DisplayName = "Synchronizing"),
	Running          UMETA(DisplayName = "Running"),
	Disconnected     UMETA(DisplayName = "Disconnected"),
	Disconnecting    UMETA(DisplayName = "Disconnecting"),
 };

USTRUCT(BlueprintType)
struct FPlayerConnectionInfo {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EGGPOPlayerType     type;
	int32               handle; // xxx: GGPOPlayerHandle
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EPlayerConnectState state;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32               connect_progress;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32               disconnect_timeout;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32               disconnect_start;
};

struct NonGameState {
	struct ChecksumInfo {
		int framenumber;
		int checksum;
	};

	void SetConnectState(GGPOPlayerHandle handle, EPlayerConnectState state);

	void SetDisconnectTimeout(GGPOPlayerHandle handle, int when, int timeout);

	void SetConnectState(EPlayerConnectState state);

	void UpdateConnectProgress(GGPOPlayerHandle handle, int progress);

	GGPOPlayerHandle     local_player_handle;
	FPlayerConnectionInfo players[MAX_PLAYERS];
	int                  num_players;

	ChecksumInfo         now;
	ChecksumInfo         periodic;

public:
	float GetDisconnectTime(int32 index) const;
};