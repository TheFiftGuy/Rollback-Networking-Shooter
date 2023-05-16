// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GGPOGame/GGPOGame.h"
#include "PlayerPawn.h"
#include "GameFramework/GameStateBase.h"
#include "GGPOGameStateBase.generated.h"
/**
 * 
 */

//General Class structure and usage of GGPO is based of the GGPO UE4 port plugin & Demo game example
//Plugin: https://github.com/BwdYeti/GGPOUE4
//game: https://github.com/BwdYeti/VectorWarUE4

UCLASS()
class SERVERROLLBACK_API AGGPOGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
private:
	GGPOSession* ggpo = nullptr;
	
	NonGameState ngs = { 0 };
	
	bool bSessionStarted;
	float ElapsedTime = 0.f;

public:
	GameState gs = { 0 };

	UPROPERTY(Category="Player",EditAnywhere,BlueprintReadWrite)
	TSubclassOf<APlayerPawn> PawnClass;
	
	UPROPERTY()
	TArray<APlayerPawn*> PlayerPawns;
	int LocalPlayerIndex = 0;
	
	AGGPOGameStateBase();
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	/**
	 * Called from BeginPlay() after creating the game state.
	 * Can be overridden by a blueprint to create actors that represent the game state.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GGPO")
	void OnSessionStarted();
	virtual void OnSessionStarted_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GGPO")
	void SetupHUD();
	virtual void SetupHUD_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetPlayerNum() const { return gs.NumPlayers; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetPlayerHP(int Index) const { return (100 - gs.PlayerHitsReceived[Index]); }
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetPlayerHits(int Index) const { return gs.PlayerHitsDealt[Index]; }

	//NonGameState GetNonGameState() const { return ngs; }
private:
	void TickGameState();

	/** Gets the inputs from the local player. */
	int32 GetLocalInputs();

	/*
	 * Run a single frame of the game.
	 */
	void ggpoGame_RunFrame(int32 local_input);
	/*
	 * Advances the game state by exactly 1 frame using the inputs specified
	 * for player 1 and player 2.
	 */
	void ggpoGame_AdvanceFrame(int32 inputs[], int32 disconnect_flags);
	/*
	 * Spend our idle time in ggpo so it can use whatever time we have left over
	 * for its internal bookkeeping.
	 */
	void ggpoGame_Idle(int32 time);
	void ggpoGame_Exit();
	
	
	/** Starts a GGPO game session. */
	bool TryStartGGPOPlayerSession(int32 NumPlayers, const UGGPONetwork* NetworkAddresses);
	
	//D This was:
	/* ggpo game init --
	 *
	 * Initialize the external game.  This initializes the game state and
	 * creates a new network session.
	 */
	void Game_Init(uint16 localport, int32 num_players, GGPOPlayer* players, int32 num_spectators);

	void UELogGameState(const int32 inputs[]);

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
