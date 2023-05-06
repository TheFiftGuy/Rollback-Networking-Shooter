#include "GameState.h"
#include "BulletMain.h"
#include "BulletHelpers.h"
#include "BulletCustomMotionState.h"
#include "BulletDebugDraw.h"

void GameState::Init()
{
	InitBullet();
}

void GameState::Update()
{
	FrameNumber++;
	
	//parse input
	//move player
	
	Bullet.BtWorld->stepSimulation(btScalar(1.) / btScalar(60.), 0);
}

GameState::~GameState()
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
