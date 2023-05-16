// Fill out your copyright notice in the Description page of Project Settings.


#include "GGPOGameStateBase.h"
#include "GGPOGameInstance.h"
#include "BulletPlayerController.h"
#include "PhysicsWorldActor.h"
#include "Kismet/GameplayStatics.h"
#include "ServerRollback/ServerRollback.h"

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
		// Reset the game instance network addresses
		GgpoGameInstance->NetworkAddresses = nullptr;
	}

	bSessionStarted = TryStartGGPOPlayerSession(NumPlayers, NetworkAddresses);

	if (bSessionStarted)
	{
		OnSessionStarted();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create GGPO session"));
	}
}

void AGGPOGameStateBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
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
	//connects the player to the pawn (camera)
	if(PlayerPawns.Num() > 0)	{
		//UGameplayStatics::GetPlayerController(GetWorld(), 0)->UnPossess();
		UGameplayStatics::GetPlayerController(GetWorld(), 0)->Possess(PlayerPawns[LocalPlayerIndex]);
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue, FString::Printf(TEXT("Possed Pawn#: %d"), LocalPlayerIndex));
	}
	SetupHUD();
}

void AGGPOGameStateBase::SetupHUD_Implementation()
{
}

void AGGPOGameStateBase::TickGameState()
{
	//UE_LOG(LogTemp, Warning, TEXT("AGGPOGameStateBase::TickGameState()"));

	int32 Input = GetLocalInputs();
	ggpoGame_RunFrame(Input);

	
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
	
	if (ngs.local_player_handle != GGPO_INVALID_HANDLE) {
#if defined(SYNC_TEST)
		local_input = rand(); // test: use random inputs to demonstrate sync testing
#endif
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
	UE_LOG(GGPOlog, Log, TEXT("%d Advance Frame, Frame# %d++."), LocalPlayerIndex, gs.FrameNumber);

	gs.Update(inputs, disconnect_flags);
	
	// update the checksums to display in the top of the window.  this
	// helps to detect desyncs.
	ngs.now.framenumber = gs.FrameNumber;
	ngs.now.checksum = fletcher32_checksum((short*)&gs, sizeof(gs) / 2);
	if ((gs.FrameNumber % 90) == 0) {
		ngs.periodic = ngs.now;
	}
	
	//log post-tick gamestate into UE log so that it can be compared to any rollbacked post-tick state
	UELogGameState(inputs);
	// Notify ggpo that we've moved forward exactly 1 frame.
	GGPONet::ggpo_advance_frame(ggpo);
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

void AGGPOGameStateBase::UELogGameState(const int32 inputs[])
{
	UE_LOG(GGPOlog, Log, TEXT("---------------LOG START ----------------------"));
	UE_LOG(GGPOlog, Log, TEXT("Frame# %d\t Players: %d.\t PLAYER LOGS BELOW."), gs.FrameNumber, gs.NumPlayers);
	for(int i = 0; i < gs.NumPlayers; i++)	{
		UE_LOG(GGPOlog, Log, TEXT("Player START-------------------------"));
		UE_LOG(GGPOlog, Log, TEXT("PLAYER %d INFO\tHP: %d\t Hits:%d\t INPUT: %d"), i+1, GetPlayerHP(i), GetPlayerHits(i), inputs[i]);
		btRigidBody* player = gs.Bullet.BtPlayerBodies[i];
		FString s1 = FString::SanitizeFloat(player->getWorldTransform().getOrigin().getX(), 5);
		FString s2 = FString::SanitizeFloat(player->getWorldTransform().getOrigin().getY(), 5);
		FString s3 = FString::SanitizeFloat(player->getWorldTransform().getOrigin().getZ(), 5);
		UE_LOG(GGPOlog, Log, TEXT("POS: %s\t%s\t%s"),  *s1, *s2, *s3);
		btScalar fl[3];
		player->getWorldTransform().getRotation().getEulerZYX(fl[0], fl[1], fl[2]);
		s1 = FString::SanitizeFloat(fl[0]);
		s2 = FString::SanitizeFloat(fl[1]);
		s3 = FString::SanitizeFloat(fl[2]);
		UE_LOG(GGPOlog, Log, TEXT("ROT: %s\t%s\t%s"),  *s1, *s2, *s3);
		s1 = FString::SanitizeFloat(player->getLinearVelocity().getX(), 5);
		s2 = FString::SanitizeFloat(player->getLinearVelocity().getY(), 5);
		s3 = FString::SanitizeFloat(player->getLinearVelocity().getZ(), 5);
		UE_LOG(GGPOlog, Log, TEXT("VEL: %s\t%s\t%s"),  *s1, *s2, *s3);		
	}

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
	UE_LOG(GGPOlog, Log, TEXT("%d Saving State, Frame# %d."), LocalPlayerIndex, gs.FrameNumber);
	gs.SaveBtBodyData();
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
	gs.LoadBtBodyData();
	UE_LOG(GGPOlog, Warning, TEXT("%d Loaded State #%d"), LocalPlayerIndex, gs.FrameNumber);

    return true;
}
bool AGGPOGameStateBase::ggpoGame_log_game_state(char* filename, unsigned char* buffer, int32)
{
   //Sadly doesnt seem to work in the UE4 GGPO plugin, no matter how I do it.
    return true;
}
void AGGPOGameStateBase::ggpoGame_free_buffer(void* buffer)
{
    free(buffer);
}
bool AGGPOGameStateBase::ggpoGame_advance_frame_callback(int32)
{
    int inputs[4] = { 0, 0, 0, 0 };
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