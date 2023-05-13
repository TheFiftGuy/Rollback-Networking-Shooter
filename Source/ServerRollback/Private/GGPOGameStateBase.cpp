// Fill out your copyright notice in the Description page of Project Settings.


#include "GGPOGameStateBase.h"
#include "GGPOGameInstance.h"
#include "BulletPlayerController.h"
#include "PhysicsWorldActor.h"
#include "Kismet/GameplayStatics.h"

#define FRAME_RATE 60
#define ONE_FRAME (1.0f / FRAME_RATE)

AGGPOGameStateBase::AGGPOGameStateBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGGPOGameStateBase::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("AGGPOGameStateBase::BeginPlay()"));

	UGGPONetwork* NetworkAddresses = nullptr;
	int32 NumPlayers = 1;
	
	if (UGGPOGameInstance* GgpoGameInstance = Cast<UGGPOGameInstance>(GetGameInstance()))
	{
		// Get the network addresses
		NetworkAddresses = GgpoGameInstance->NetworkAddresses;
		NumPlayers = NetworkAddresses->NumPlayers();
		//GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue, FString::Printf(TEXT("NumPlayers = %d"), NumPlayers));
		// Reset the game instance network addresses
		GgpoGameInstance->NetworkAddresses = nullptr;
	}

	bSessionStarted = TryStartGGPOPlayerSession(NumPlayers, NetworkAddresses);

	if (bSessionStarted)
	{
		OnSessionStarted();
		//D TODO  Render network graph stuff(taken from VW port).
		/*
		NetworkGraphData.Empty();
		TArray<FGGPONetworkStats> Network = VectorWar_GetNetworkStats();
		int32 Count = Network.Num();
		for (int32 i = 0; i < Count; i++)
		{
			NetworkGraphData.Add(FNetworkGraphPlayer{ });
		}
		*/
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create GGPO session"));
	}
}

void AGGPOGameStateBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	//UE_LOG(LogTemp, Warning, TEXT("AGGPOGameStateBase::Tick"));

	MSG msg = { 0 };

	ElapsedTime += DeltaSeconds;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_QUIT) { }
	}
	int32 IdleMs = (int32)(ONE_FRAME - (int32)(ElapsedTime * 1000));
	ggpoGame_Idle(FMath::Max(0, IdleMs - 1));
	while (ElapsedTime >= ONE_FRAME) {
		TickGameState();

		ElapsedTime -= ONE_FRAME;
	}
}

void AGGPOGameStateBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (bSessionStarted)
	{
		ggpoGame_Exit();

		bSessionStarted = false;
	}
}

void AGGPOGameStateBase::OnSessionStarted_Implementation()
{
	/*FActorSpawnParameters SpawnInfo;
	FVector SpawnLoc = FVector(0,0,100);
	FRotator SpawnRot = FRotator();
	for (int i = 0; i < gs.NumPlayers; i++)
	{
		SpawnLoc.X = 100*i; //offset pos
		PlayerPawns.Add(GetWorld()->SpawnActor<APlayerPawn>(PawnClass, SpawnLoc, SpawnRot, SpawnInfo));
	}*/
	//connects the player to the pawn (camera)
	if(PlayerPawns.Num() > 0)	{
		//UGameplayStatics::GetPlayerController(GetWorld(), 0)->UnPossess();
		UGameplayStatics::GetPlayerController(GetWorld(), 0)->Possess(PlayerPawns[LocalPlayerIndex]);
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue, FString::Printf(TEXT("Possed Pawn#: %d"), LocalPlayerIndex));
	}
		
}

void AGGPOGameStateBase::TickGameState()
{
	//UE_LOG(LogTemp, Warning, TEXT("AGGPOGameStateBase::TickGameState()"));

	int32 Input = GetLocalInputs();
	ggpoGame_RunFrame(Input);

	//D TODO: Network Data stuff can go here
	/*TArray<FGGPONetworkStats> Network = VectorWar_GetNetworkStats();
	for (int32 i = 0; i < NetworkGraphData.Num(); i++)
	{
		TArray<FNetworkGraphData>* PlayerData = &NetworkGraphData[i].PlayerData;

		int32 Fairness;
		int32 LocalFairness = Network[i].timesync.local_frames_behind;
		int32 RemoteFairness = Network[i].timesync.remote_frames_behind;
		int32 Ping = Network[i].network.ping;

		if (LocalFairness < 0 && RemoteFairness < 0) {
			/*
			 * Both think it's unfair (which, ironically, is fair).  Scale both and subtrace.
			 #1#
			Fairness = abs(abs(LocalFairness) - abs(RemoteFairness));
		}
		else if (LocalFairness > 0 && RemoteFairness > 0) {
			/*
			 * Impossible!  Unless the network has negative transmit time.  Odd....
			 #1#
			Fairness = 0;
		}
		else {
			/*
			 * They disagree.  Add.
			 #1#
			Fairness = abs(LocalFairness) + abs(RemoteFairness);
		}

		FNetworkGraphData GraphData = FNetworkGraphData{ Fairness, RemoteFairness, Ping };
		PlayerData->Add(GraphData);

		while (PlayerData->Num() > NETWORK_GRAPH_STEPS)
		{
			PlayerData->RemoveAt(0);
		}
	}*/
}

int32 AGGPOGameStateBase::GetLocalInputs()
{
	
	if(ABulletPlayerController* BulletController = Cast<ABulletPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0)))
	{
		return BulletController->GetBulletInput();
	}
	//else return blank
	return 0;
}

void AGGPOGameStateBase::ggpoGame_RunFrame(int32 local_input)
{
//	UE_LOG(LogTemp, Warning, TEXT("AGGPOGameStateBase::ggpoGame_RunFrame"));

	GGPOErrorCode result = GGPO_OK;
	int disconnect_flags;
	int32 inputs[GGPO_MAX_PLAYERS] = { 0 };

	//D TODO: Non-game-state and localPlayerHandle
	if (ngs.local_player_handle != GGPO_INVALID_HANDLE) {
		result = GGPONet::ggpo_add_local_input(ggpo, ngs.local_player_handle, &local_input, sizeof(local_input));
	}

	// synchronize these inputs with ggpo.  If we have enough input to proceed
	// ggpo will modify the input list with the correct inputs to use and return 1.
	if (GGPO_SUCCEEDED(result)) {
		result = GGPONet::ggpo_synchronize_input(ggpo, (void*)inputs, sizeof(int) * GGPO_MAX_PLAYERS, &disconnect_flags);
		if (GGPO_SUCCEEDED(result)) {
			// inputs[0] and inputs[1] contain the inputs for p1 and p2.  Advance
			// the game by 1 frame using those inputs.
			ggpoGame_AdvanceFrame(inputs, disconnect_flags);
		}
	}
}

void AGGPOGameStateBase::ggpoGame_AdvanceFrame(int32 inputs[], int32 disconnect_flags)
{
	//UE_LOG(LogTemp, Warning, TEXT("AGGPOGameStateBase::ggpoGame_AdvanceFrame"));

	gs.Update(inputs, disconnect_flags);
	
	// update the checksums to display in the top of the window.  this
	// helps to detect desyncs.
	ngs.now.framenumber = gs.FrameNumber;
	ngs.now.checksum = fletcher32_checksum((short*)&gs, sizeof(gs) / 2);
	if ((gs.FrameNumber % 90) == 0) {
		ngs.periodic = ngs.now;
	}

	// Notify ggpo that we've moved forward exactly 1 frame.
	GGPONet::ggpo_advance_frame(ggpo);

	// Update the performance monitor display.
	GGPOPlayerHandle handles[MAX_PLAYERS];
	int count = 0;
	for (int i = 0; i < ngs.num_players; i++) {
		if (ngs.players[i].type == EGGPOPlayerType::REMOTE) {
			handles[count++] = ngs.players[i].handle;
		}
	}
}

void AGGPOGameStateBase::ggpoGame_Idle(int32 time)
{
	GGPONet::ggpo_idle(ggpo, time);
}

void AGGPOGameStateBase::ggpoGame_Exit()
{
	gs.OnDestroy();
	memset(&gs, 0, sizeof(gs));
	memset(&ngs, 0, sizeof(ngs));
	if (ggpo) {
		GGPONet::ggpo_close_session(ggpo);
		ggpo = NULL;
	}
}

bool AGGPOGameStateBase::TryStartGGPOPlayerSession(int32 NumPlayers, const UGGPONetwork* NetworkAddresses)
{
	UE_LOG(LogTemp, Warning, TEXT("AGGPOGameStateBase::TryStartGGPOPlayerSession"));
	int32 Offset = 0;
	GGPOPlayer Players[GGPO_MAX_SPECTATORS + GGPO_MAX_PLAYERS];
	int32 NumSpectators = 0;

	uint16 LocalPort;

	// If there are no 
	if (NetworkAddresses == nullptr)
	{
		Players[0].size = sizeof(Players[0]);
		Players[0].player_num = 1;
		Players[0].type = EGGPOPlayerType::LOCAL;

		LocalPort = 7000;
		NumPlayers = 1;
	}
	else
	{
		if (NumPlayers > NetworkAddresses->NumPlayers())
			return false;

		LocalPort = NetworkAddresses->GetLocalPort();

		int32 i;
		for (i = 0; i < NumPlayers; i++)
		{
			Offset++;

			Players[i].size = sizeof(Players[i]);
			Players[i].player_num = i + 1;
			// The local player
			if (i == NetworkAddresses->GetPlayerIndex()) {
				Players[i].type = EGGPOPlayerType::LOCAL;
				LocalPlayerIndex = i;
				continue;
			}

			Players[i].type = EGGPOPlayerType::REMOTE;
			Players[i].u.remote.port = (uint16)NetworkAddresses->GetAddress(i)->GetPort();
			NetworkAddresses->GetAddress(i)->GetIpAddress(Players[i].u.remote.ip_address);
		}
		// these are spectators...
		while (Offset < NetworkAddresses->NumPlayers()) {
			Offset++;

			Players[i].type = EGGPOPlayerType::SPECTATOR;
			Players[i].u.remote.port = (uint16)NetworkAddresses->GetAddress(i)->GetPort();
			NetworkAddresses->GetAddress(i)->GetIpAddress(Players[i].u.remote.ip_address);

			i++;
			NumSpectators++;
		}
	}

	Game_Init(LocalPort, NumPlayers, Players, NumSpectators);

	UE_LOG(LogTemp, Display, TEXT("GGPO session started"));

	return true;
}

void AGGPOGameStateBase::Game_Init(uint16 localport, int32 num_players, GGPOPlayer* players, int32 num_spectators)
{
	UE_LOG(LogTemp, Warning, TEXT("AGGPOGameStateBase::Game_Init"));
	
	GGPOErrorCode result;
	// Initialize the game state
	gs.Init(num_players);
	ngs.num_players = num_players;
	
	if(APhysicsWorldActor* PhysWorldActor = Cast<APhysicsWorldActor>(UGameplayStatics::GetActorOfClass(GetWorld(), APhysicsWorldActor::StaticClass())))
	{
		PhysWorldActor->InitPhysWorld();
		/*if(PlayerPawns.Num() > 0)
		{
			for (APlayerPawn* pawn : PlayerPawns)
			{
				PhysWorldActor->AddPhysicsPlayer(pawn);
			}
			UGameplayStatics::GetPlayerController(GetWorld(), 0)->Possess(PlayerPawns[LocalPlayerIndex]);
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue, FString::Printf(TEXT("Possed Pawn#: %d"), LocalPlayerIndex));
		}*/
		FActorSpawnParameters SpawnInfo;
		FVector SpawnLoc = FVector(0,0,150);
		FRotator SpawnRot = FRotator();
		
		for (int i = 0; i < num_players; i++)
		{
			
			SpawnLoc.Y = 200*i; //offset pos
			PlayerPawns.Add(GetWorld()->SpawnActor<APlayerPawn>(PawnClass, SpawnLoc, SpawnRot, SpawnInfo));
			PhysWorldActor->AddPhysicsPlayer(PlayerPawns[i]);
			UE_LOG(LogTemp, Warning, TEXT("Added Player Body"));
		}
	}
	// Fill in a ggpo callbacks structure to pass to start_session.
	GGPOSessionCallbacks cb = CreateCallbacks();

#if defined(SYNC_TEST)
	result = GGPONet::ggpo_start_synctest(&ggpo, &cb, "ggpoGame", num_players, sizeof(int), 1);
#else
	result = GGPONet::ggpo_start_session(&ggpo, &cb, "ggpoGame", num_players, sizeof(int), localport);
#endif

	if(result == GGPO_OK)
	{
		UE_LOG(LogTemp, Warning, TEXT("ggpo_start_session OK"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ggpo_start_session NOT ok"));

	}
	
	// automatically disconnect clients after 3000 ms and start our count-down timer
	// for disconnects after 1000 ms.   To completely disable disconnects, simply use
	// a value of 0 for ggpo_set_disconnect_timeout.
	GGPONet::ggpo_set_disconnect_timeout(ggpo, 3000);
	GGPONet::ggpo_set_disconnect_notify_start(ggpo, 1000);

	
	for (int i = 0; i < num_players + num_spectators; i++) {
		GGPOPlayerHandle handle;
		result = GGPONet::ggpo_add_player(ggpo, players + i, &handle);
		ngs.players[i].handle = handle;
		ngs.players[i].type = players[i].type;
		if (players[i].type == EGGPOPlayerType::LOCAL) {
			ngs.players[i].connect_progress = 100;
			ngs.local_player_handle = handle;
			ngs.SetConnectState(handle, EPlayerConnectState::Connecting);
			GGPONet::ggpo_set_frame_delay(ggpo, handle, FRAME_DELAY);

		}
		else {
			ngs.players[i].connect_progress = 0;
		}
	}

	GGPONet::ggpo_try_synchronize_local(ggpo);
}

GGPOSessionCallbacks AGGPOGameStateBase::CreateCallbacks()
{
	GGPOSessionCallbacks cb = { 0 };

	cb.begin_game = std::bind(&AGGPOGameStateBase::ggpoGame_begin_game_callback, this, std::placeholders::_1);
	cb.save_game_state = std::bind(&AGGPOGameStateBase::ggpoGame_save_game_state_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	cb.load_game_state = std::bind(&AGGPOGameStateBase::ggpoGame_load_game_state_callback, this, std::placeholders::_1, std::placeholders::_2);
	cb.log_game_state = std::bind(&AGGPOGameStateBase::ggpoGame_log_game_state, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	cb.free_buffer = std::bind(&AGGPOGameStateBase::ggpoGame_free_buffer, this, std::placeholders::_1);
	cb.advance_frame = std::bind(&AGGPOGameStateBase::ggpoGame_advance_frame_callback, this, std::placeholders::_1);
	cb.on_event = std::bind(&AGGPOGameStateBase::ggpoGame_on_event_callback, this, std::placeholders::_1);

	return cb;
}


bool AGGPOGameStateBase::ggpoGame_begin_game_callback(const char*)
{
    return true;
}
bool AGGPOGameStateBase::ggpoGame_save_game_state_callback(unsigned char** buffer, int32* len, int32* checksum, int32)
{
    *len = sizeof(gs);
    *buffer = (unsigned char*)malloc(*len);
    if (!*buffer) {
        return false;
    }
    memcpy(*buffer, &gs, *len);
    *checksum = fletcher32_checksum((short*)*buffer, *len / 2);
    return true;
}
bool AGGPOGameStateBase::ggpoGame_load_game_state_callback(unsigned char* buffer, int32 len)
{
    memcpy(&gs, buffer, len);
    return true;
}
bool AGGPOGameStateBase::ggpoGame_log_game_state(char* filename, unsigned char* buffer, int32)
{
    /*FILE* fp = nullptr;
    fopen_s(&fp, filename, "w");
    if (fp) {
        GameState* gamestate = (GameState*)buffer;
        fprintf(fp, "GameState object.\n");
        fprintf(fp, "  bounds: %ld,%ld x %ld,%ld.\n", gamestate->_bounds.left, gamestate->_bounds.top,
            gamestate->_bounds.right, gamestate->_bounds.bottom);
        fprintf(fp, "  num_ships: %d.\n", gamestate->_num_ships);
        for (int i = 0; i < gamestate->_num_ships; i++) {
            Ship* ship = gamestate->_ships + i;
            fprintf(fp, "  ship %d position:  %.4f, %.4f\n", i, ship->position.x, ship->position.y);
            fprintf(fp, "  ship %d velocity:  %.4f, %.4f\n", i, ship->velocity.dx, ship->velocity.dy);
            fprintf(fp, "  ship %d radius:    %d.\n", i, ship->radius);
            fprintf(fp, "  ship %d heading:   %d.\n", i, ship->heading);
            fprintf(fp, "  ship %d health:    %d.\n", i, ship->health);
            fprintf(fp, "  ship %d speed:     %d.\n", i, ship->speed);
            fprintf(fp, "  ship %d cooldown:  %d.\n", i, ship->cooldown);
            fprintf(fp, "  ship %d score:     %d.\n", i, ship->score);
            for (int j = 0; j < MAX_BULLETS; j++) {
                Bullet* bullet = ship->bullets + j;
                fprintf(fp, "  ship %d bullet %d: %.2f %.2f -> %.2f %.2f.\n", i, j,
                    bullet->position.x, bullet->position.y,
                    bullet->velocity.dx, bullet->velocity.dy);
            }
        }
        fclose(fp);
    }*/
    return true;
}
void AGGPOGameStateBase::ggpoGame_free_buffer(void* buffer)
{
    free(buffer);
}
bool AGGPOGameStateBase::ggpoGame_advance_frame_callback(int32)
{
    int inputs[4] = { 0 };
    int disconnect_flags;

    // Make sure we fetch new inputs from GGPO and use those to update
    // the game state instead of reading from the keyboard.
    GGPONet::ggpo_synchronize_input(ggpo, (void*)inputs, sizeof(int) * 4, &disconnect_flags);
    ggpoGame_AdvanceFrame(inputs, disconnect_flags);
    return true;
}
bool AGGPOGameStateBase::ggpoGame_on_event_callback(GGPOEvent* info)
{
    int progress;
    switch (info->code) {
    case GGPO_EVENTCODE_CONNECTED_TO_PEER:
        ngs.SetConnectState(info->u.connected.player, EPlayerConnectState::Synchronizing);
        break;
    case GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER:
        progress = 100 * info->u.synchronizing.count / info->u.synchronizing.total;
        ngs.UpdateConnectProgress(info->u.synchronizing.player, progress);
        break;
    case GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER:
        ngs.UpdateConnectProgress(info->u.synchronized.player, 100);
        break;
    case GGPO_EVENTCODE_RUNNING:
        ngs.SetConnectState(EPlayerConnectState::Running);
        break;
    case GGPO_EVENTCODE_CONNECTION_INTERRUPTED:
        ngs.SetDisconnectTimeout(info->u.connection_interrupted.player,
            get_time(),
            info->u.connection_interrupted.disconnect_timeout);
        break;
    case GGPO_EVENTCODE_CONNECTION_RESUMED:
        ngs.SetConnectState(info->u.connection_resumed.player, EPlayerConnectState::Running);
        break;
    case GGPO_EVENTCODE_DISCONNECTED_FROM_PEER:
        ngs.SetConnectState(info->u.disconnected.player, EPlayerConnectState::Disconnected);
        break;
    case GGPO_EVENTCODE_TIMESYNC:
        Sleep(1000 * info->u.timesync.frames_ahead / 60);
        break;
    }
    return true;
}