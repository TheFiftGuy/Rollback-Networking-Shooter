// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "include/ggponet.h"


#include "GameFramework/GameStateBase.h"
#include "GGPOGameStateBase.generated.h"

/**
 * 
 */
UCLASS()
class SERVERROLLBACK_API AGGPOGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
private:
	GGPOSession* ggpo = nullptr;


	bool bSessionStarted;


public:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;


	/**
	 * Called from BeginPlay() after creating the game state.
	 * Can be overridden by a blueprint to create actors that represent the game state.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GGPO")
	void OnSessionStarted();
	virtual void OnSessionStarted_Implementation();

private:
	/** Starts a GGPO game session. */
	bool TryStartGGPOPlayerSession(int32 NumPlayers, const UGGPONetwork* NetworkAddresses);
	
	//D This was:
	/* VectorWar_Init --
	 *
	 * Initialize the vector war game.  This initializes the game state and
	 * creates a new network session.
	 */
	void Game_Init(uint16 localport, int32 num_players, GGPOPlayer* players, int32 num_spectators);

private:
	//D GGPO CALLBACKS SECTION ------------------------------------
	/** Gets a GGPOSessionCallbacks object with its callback functions assigned. */
	GGPOSessionCallbacks CreateCallbacks();
	
	/*
	 * The begin game callback.  We don't need to do anything special here,
	 * so just return true.
	 */
	bool __cdecl ggpoGame_begin_game_callback(const char*);
	/*
	 * Save the current state to a buffer and return it to GGPO via the
	 * buffer and len parameters.
	 */
	bool __cdecl ggpoGame_save_game_state_callback(unsigned char** buffer, int32* len, int32* checksum, int32);
	/*
	 * Makes our current state match the state passed in by GGPO.
	 */
	bool __cdecl ggpoGame_load_game_state_callback(unsigned char* buffer, int32 len);
	/*
	 * Log the gamestate.  Used by the synctest debugging tool.
	 */
	bool __cdecl ggpoGame_log_game_state(char* filename, unsigned char* buffer, int32);
	/*
	 * Free a save state buffer previously returned in vw_save_game_state_callback.
	 */
	void __cdecl ggpoGame_free_buffer(void* buffer);
	/*
	 * Notification from GGPO we should step foward exactly 1 frame
	 * during a rollback.
	 */
	bool __cdecl ggpoGame_advance_frame_callback(int32);
	/*
	 * Notification from GGPO that something has happened.  Update the status
	 * text at the bottom of the screen to notify the user.
	 */
	bool __cdecl ggpoGame_on_event_callback(GGPOEvent* info);
};
