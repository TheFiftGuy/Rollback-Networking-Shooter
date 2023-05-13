#include "GameState.h"
#include "BulletMain.h"
#include "BulletHelpers.h"
#include "BulletCustomMotionState.h"
#include "BulletDebugDraw.h"
#include "GGPOGame.h"

void GameState::Init(int NumPlayers_)
{
	NumPlayers = NumPlayers_;
	InitBullet();
}

void GameState::GetPlayerAI(int PlayerIndex, FVector* outPlayerMovement, FVector2D* outMouseDelta, bool* outFire)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue, FString::Printf(TEXT("Get Player %d AI."), PlayerIndex));

	*outPlayerMovement = BulletHelpers::ToUEDir(Bullet.BtPlayerBodies[PlayerIndex]->getLinearVelocity()/10, false);
	*outMouseDelta = FVector2D(0);
	*outFire = false;
}

void GameState::ParsePlayerInputs(int32 Inputs, int PlayerIndex, FVector* outPlayerMovement, FVector2D* outMouseDelta, bool* outFire)
{
	bool Forwards = Inputs & INPUT_FORWARDS;
	bool Backwards = Inputs & INPUT_BACKWARDS;
	bool Left = Inputs & INPUT_LEFT;
	bool Right = Inputs & INPUT_RIGHT;
	//F or B
	if(Forwards && Backwards)
	{
		outPlayerMovement->X = 0;
	}
	else if(Forwards)
	{
		outPlayerMovement->X = 1;
	}
	else if(Backwards)
	{
		outPlayerMovement->X = -1;
	}
	//L or R
	if(Left && Right)
	{
		outPlayerMovement->Y = 0;
	}
	else if(Right)
	{
		outPlayerMovement->Y = 1;
	}
	else if(Left)
	{
		outPlayerMovement->Y = -1;
	}
	outPlayerMovement->Z = ((Inputs & INPUT_JUMP) ? 1 : 0);
	
	/*int32 MouseDeltaX = ((Inputs >> 8) & 0x1FFF) | (((Inputs >> 20) & 0x1) ? 0xFFFFE000 : 0);
	int32 MouseDeltaY = ((Inputs >> 21) & 0x1FFF) | (((Inputs >> 33) & 0x1) ? 0xFFFFE000 : 0);
	*outMouseDelta = FVector2D(MouseDeltaX / 8192.0f, MouseDeltaY / 8192.0f);*/
	int32 MouseDeltaX = ((Inputs >> 6) & 0x1FFF);
	int32 MouseDeltaY = ((Inputs >> 19) & 0x1FFF);
	MouseDeltaX |= ((MouseDeltaX & 0x1000) ? 0xFFFFE000 : 0);
	MouseDeltaY |= ((MouseDeltaY & 0x1000) ? 0xFFFFE000 : 0);
	*outMouseDelta = FVector2D(MouseDeltaX / 4096.0f, MouseDeltaY / 4096.0f);

	
	*outFire = Inputs & INPUT_FIRE;
}

void GameState::ApplyInputToPlayer(int PlayerIndex, FVector* outPlayerMovement, FVector2D* outMouseDelta, bool* outFire)
{
	Bullet.BtPlayerBodies[PlayerIndex]->setLinearVelocity(BulletHelpers::ToBtDir(*outPlayerMovement * 10.f, false));
	//GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue, FString::Printf(TEXT("Pawn# %d velocity: %s"), Bullet.BtPlayerBodies.Num(), *outPlayerMovement->ToString()));
}

void GameState::Update(int inputs[], int disconnect_flags)
{
	//parse input
	//move player
	FrameNumber++;
	for (int i = 0; i < NumPlayers; i++) {
		FVector PlayerMovement = FVector();
		FVector2D MouseDelta = FVector2D();
		bool Fire = false;

		if (disconnect_flags & (1 << i)) {
			GetPlayerAI(i, &PlayerMovement, &MouseDelta, &Fire);
		} else {
			ParsePlayerInputs(inputs[i], i, &PlayerMovement, &MouseDelta, &Fire);
		}
		
		ApplyInputToPlayer(i, &PlayerMovement, &MouseDelta, &Fire);
	}
	
	
	Bullet.BtWorld->stepSimulation(btScalar(1.) / btScalar(60.), 0);
}

void GameState::OnDestroy()
{
	if(Bullet.BtWorld)
	{
		if(Bullet.BtWorld->getNumCollisionObjects() - 1 >= 0)
		{
			for (int i = Bullet.BtWorld->getNumCollisionObjects() - 1; i >= 0; i--)
			{
				btCollisionObject* obj = Bullet.BtWorld->getCollisionObjectArray()[i];
				btRigidBody* body = btRigidBody::upcast(obj);
				if (body && body->getMotionState())
				{
					delete body->getMotionState();
				}
				Bullet.BtWorld->removeCollisionObject(obj);
				delete obj;
			}
		}
	}
	
	
	// delete collision shapes
	for (int i = 0; i < Bullet.BtBoxCollisionShapes.Num(); i++)
		delete Bullet.BtBoxCollisionShapes[i];
	Bullet.BtBoxCollisionShapes.Empty();
	for (int i = 0; i < Bullet.BtSphereCollisionShapes.Num(); i++)
		delete Bullet.BtSphereCollisionShapes[i];
	Bullet.BtSphereCollisionShapes.Empty();
	for (int i = 0; i < Bullet.BtCapsuleCollisionShapes.Num(); i++)
		delete Bullet.BtCapsuleCollisionShapes[i];
	Bullet.BtCapsuleCollisionShapes.Empty();
	for (int i = 0; i < Bullet.BtConvexHullCollisionShapes.Num(); i++)
		delete Bullet.BtConvexHullCollisionShapes[i].Shape;
	Bullet.BtConvexHullCollisionShapes.Empty();
	for (int i = 0; i < Bullet.CachedDynamicShapes.Num(); i++)
	{
		// Only delete if this is a compound shape, otherwise it's an alias to other simple arrays
		if (Bullet.CachedDynamicShapes[i].bIsCompound)
			delete Bullet.CachedDynamicShapes[i].Shape;
	}
	Bullet.CachedDynamicShapes.Empty();

	delete Bullet.BtWorld;
	delete Bullet.BtConstraintSolver;
	delete Bullet.BtBroadphase;
	delete Bullet.BtCollisionDispatcher;
	delete Bullet.BtCollisionConfig;
	//delete BtDebugDraw;

	Bullet.BtWorld = nullptr;
	Bullet.BtConstraintSolver = nullptr;
	Bullet.BtBroadphase = nullptr;
	Bullet.BtCollisionDispatcher = nullptr;
	Bullet.BtCollisionConfig = nullptr;
	//BtDebugDraw = nullptr;

	// Clear our type-specific arrays (duplicate refs)
	Bullet.BtStaticObjects.Empty();
	Bullet.BtRigidBodies.Empty();
	Bullet.BtPlayerBodies.Empty();
}

void GameState::InitBullet()
{
	// This is all pretty standard Bullet bootstrap
	Bullet.BtCollisionConfig = new btDefaultCollisionConfiguration();
	Bullet.BtCollisionDispatcher = new btCollisionDispatcher (Bullet.BtCollisionConfig);
	Bullet.BtBroadphase = new btDbvtBroadphase ();
	Bullet.BtConstraintSolver = new btSequentialImpulseConstraintSolver();
	Bullet.BtWorld = new btDiscreteDynamicsWorld (Bullet.BtCollisionDispatcher, Bullet.BtBroadphase, Bullet.BtConstraintSolver, Bullet.BtCollisionConfig);

	// I mess with a few settings on BtWorld->getSolverInfo() but they're specific to my needs	

	// Gravity vector in our units (1=1cm)
	Bullet.BtWorld->setGravity(BulletHelpers::ToBtDir(FVector(0, 0, -980)));
}
