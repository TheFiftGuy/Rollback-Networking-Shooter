#pragma once
#include "BulletMinimal.h"
#include <BulletDynamics/Dynamics/btRigidBody.h>
// UE4: allow Windows platform types to avoid naming collisions
//  this must be undone at the bottom of this file
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/prewindowsapi.h"

#include <windef.h>


class APlayerPawn;
class btDiscreteDynamicsWorld;

//Contains All bulletphysics data
struct BulletPhysics	{
	
	// Global objects
	btCollisionConfiguration* BtCollisionConfig;
	btCollisionDispatcher* BtCollisionDispatcher;
	btBroadphaseInterface* BtBroadphase;
	btConstraintSolver* BtConstraintSolver;
	btDiscreteDynamicsWorld* BtWorld;

	// Dynamic bodies
	TArray<btRigidBody*> BtRigidBodies;
	TArray<btRigidBody*> BtPlayerBodies;

	// Static colliders
	TArray<btCollisionObject*> BtStaticObjects;
	// Re-usable collision shapes
	TArray<btBoxShape*> BtBoxCollisionShapes;
	TArray<btSphereShape*> BtSphereCollisionShapes;
	TArray<btCapsuleShape*> BtCapsuleCollisionShapes;
	// Structure to hold re-usable ConvexHull shapes based on origin BodySetup / subindex / scale
	struct ConvexHullShapeHolder
	{
		UBodySetup* BodySetup;
		int HullIndex;
		FVector Scale;
		btConvexHullShape* Shape;
	};
	TArray<ConvexHullShapeHolder> BtConvexHullCollisionShapes;
	// These shapes are for *potentially* compound rigid body shapes
	struct CachedDynamicShapeData
	{
		FName ClassName; // class name for cache
		btCollisionShape* Shape;
		bool bIsCompound; // if true, this is a compound shape and so must be deleted
		btScalar Mass;
		btVector3 Inertia; // because we like to precalc this
	};
	TArray<CachedDynamicShapeData> CachedDynamicShapes;
};

struct GameState
{
public:
	//PhysWorldActor
	//Player Pawn
	//Player Controller?
	//Other Bullet Stuff?

	void Init(int NumPlayers_);
	void GetPlayerAI(int PlayerIndex, FVector* outPlayerMovement, FVector2D* outMouseDelta, bool* outFire);
	void ParsePlayerInputs(int32 Inputs, int PlayerIndex, FVector* outPlayerMovement, FVector2D* outMouseDelta, bool* outFire);
	void ApplyInputToPlayer(int PlayerIndex, FVector* outPlayerMovement, FVector2D* outMouseDelta, bool* outFire);
	void Update(int inputs[], int disconnect_flags);

	void OnDestroy();


	int FrameNumber = 0;
	int NumPlayers = 1;
	BulletPhysics Bullet;
	//Contains PlayerBodiesData, then rest of bodiesData
	TArray<btRigidBodyFloatData> BtBodyData;
private:
	void InitBullet();
	
	//pre-simulate load data
	int LoadBtBodyData();
	//post-simulate save data
	int SaveBtBodyData();

	void DeSerializeBtBodyData(btRigidBody* OutBody, const btRigidBodyFloatData& InData);
};

// UE4: disallow windows platform types
//  this was enabled at the top of the file
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"
