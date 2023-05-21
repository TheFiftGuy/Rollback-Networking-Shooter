#include "GameState.h"
#include "BulletMain.h"
#include "BulletHelpers.h"
#include "BulletCustomMotionState.h"
#include "BulletDebugDraw.h"
#include "GGPOGame.h"
#include "PlayerPawn.h"
#include "Camera/CameraComponent.h"

void GameState::Init(int NumPlayers_)
{
	NumPlayers = NumPlayers_;
	FrameNumber = 0;
	UE_LOG(LogTemp, Log, TEXT("GameState::Init Player Quantity = %d"), NumPlayers);

	InitBullet();
}

void GameState::GetPlayerAI(int PlayerIndex, FVector* outPlayerMovement, FVector2D* outMouseDelta, bool* outFire)
{
	UE_LOG(LogTemp, Log, TEXT("Player %d AI input"), PlayerIndex);

	*outPlayerMovement = FVector(0,0,0);
	*outMouseDelta = FVector2D(0);
	*outFire = false;
}

void GameState::ParsePlayerInputs(int Inputs, int PlayerIndex, FVector* outPlayerMovement, FVector2D* outMouseDelta, bool* outFire)
{
	bool Forwards = Inputs & INPUT_FORWARDS;
	bool Backwards = Inputs & INPUT_BACKWARDS;
	bool Left = Inputs & INPUT_LEFT;
	bool Right = Inputs & INPUT_RIGHT;
	//F or B
	if(Forwards && !Backwards)
	{
		outPlayerMovement->X = 1;
	}
	else if(Backwards && !Forwards)
	{
		outPlayerMovement->X = -1;
	}
	else
	{
		outPlayerMovement->X = 0;
	}
	//L or R
	if(Right && !Left)
	{
		outPlayerMovement->Y = 1;
	}
	else if(Left && !Right)
	{
		outPlayerMovement->Y = -1;
	}
	else
	{
		outPlayerMovement->Y = 0;
	}
	outPlayerMovement->Z = ((Inputs & INPUT_JUMP) ? 1 : 0);

	int MouseDeltaX = ((Inputs >> 6) & 0x1FFF);
	int MouseDeltaY = ((Inputs >> 19) & 0x1FFF);
	MouseDeltaX |= ((MouseDeltaX & 0x1000) ? 0xFFFFE000 : 0);
	MouseDeltaY |= ((MouseDeltaY & 0x1000) ? 0xFFFFE000 : 0);
	
	float sens = 0.5;
	*outMouseDelta = FVector2D((MouseDeltaX/4096.0f)*100 *sens, (MouseDeltaY / 4096.0f)*100 *sens);
	//UE_LOG(LogTemp, Log, TEXT("Player %d Mouse input: %s "), PlayerIndex, *outMouseDelta->ToString());
	
	*outFire = Inputs & INPUT_FIRE;

}

void GameState::ApplyInputToPlayer(int PlayerIndex, FVector inPlayerMovement, FVector2D inMouseDelta, bool inFire)
{
	PlayerTurn(PlayerIndex, inMouseDelta);
	PlayerMove(PlayerIndex, inPlayerMovement);
	if(inFire && FireCooldown[PlayerIndex] <= 0)
	{
		PlayerFire(PlayerIndex);
		//reset cooldown by __ frames (60fps)
		FireCooldown[PlayerIndex] = 30;
	}
	else if(FireCooldown[PlayerIndex] > 0)	{
		FireCooldown[PlayerIndex]--;
	}
}

void GameState::Update(int inputs[], int disconnect_flags)
{	
	FrameNumber++;
	FVector PlayerMovement;	FVector2D MouseDelta; bool Fire;
	for (int i = 0; i < NumPlayers; i++) {
		PlayerMovement = FVector(0.0);
		MouseDelta = FVector2D(0.0);
		Fire = false;

		if (disconnect_flags & (1 << i)) {
			GetPlayerAI(i, &PlayerMovement, &MouseDelta, &Fire);
		} else {
			ParsePlayerInputs(inputs[i], i, &PlayerMovement, &MouseDelta, &Fire);
		}
		
		ApplyInputToPlayer(i, PlayerMovement, MouseDelta, Fire);
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
	// Gravity vector in our units (1=1cm)
	Bullet.BtWorld->setGravity(BulletHelpers::ToBtDir(FVector(0, 0, -980)));
}

void GameState::PlayerMove(int PlayerIndex, FVector PlayerMovement)
{
	btVector3 PlayerVel = Bullet.BtPlayerBodies[PlayerIndex]->getLinearVelocity();
	btVector3 BtMovement = (BulletHelpers::ToBtDir(PlayerMovement));
	//movement * forwardsVec * 600 (speed)
	BtMovement = (Bullet.BtPlayerBodies[PlayerIndex]->getWorldTransform().getBasis() * BtMovement) *600;

	
	if(BtMovement.getZ() > KINDA_SMALL_NUMBER)	{
		//is grounded
		btVector3 From = Bullet.BtPlayerBodies[PlayerIndex]->getWorldTransform().getOrigin();
		btVector3 Offset = btVector3(0.f, 0.f, - (0.96 + 0.25));
		btVector3 To = From + Offset;
		btCollisionWorld::ClosestRayResultCallback rayCallback(From, To);
		Bullet.BtWorld->rayTest(From, To, rayCallback);
		//if standing on something and not falling
		if(rayCallback.hasHit())
		{
			//UE_LOG(LogTemp, Warning, TEXT("Jump raycast hit: %hs"), rayCallback.m_collisionObject->getCollisionShape()->getName());
			BtMovement.setZ(7.5);
		}
		else	{
			BtMovement.setZ(0);
		}
	}
	
	PlayerVel.setX(BtMovement.getX());
	PlayerVel.setY(BtMovement.getY());
	//if not jumping or falling, apply BtMovement.Z, else do not change vel.z
	if(BtMovement.getZ() > 1)
		PlayerVel.setZ(BtMovement.getZ());
	
	//clamp values
	Bullet.BtPlayerBodies[PlayerIndex]->setLinearVelocity(PlayerVel);
}

void GameState::PlayerTurn(int PlayerIndex, FVector2D MouseDelta)
{
	if(APlayerPawn* Pawn = (APlayerPawn*)Bullet.BtPlayerBodies[PlayerIndex]->getUserPointer())	{
		//turn camera vert/horiz
		FRotator yaw = FRotator(0,MouseDelta.X, 0);
		Pawn->AddActorLocalRotation(yaw);
		FRotator pitch = FRotator(MouseDelta.Y, 0, 0);
		//dont apply Rotation if it hits the Max/min pitch
		if((Pawn->Camera.Get()->GetRelativeRotation().Pitch + MouseDelta.Y) >= 89 ||(Pawn->Camera.Get()->GetRelativeRotation().Pitch + MouseDelta.Y) <= -89)	{}
		else
		{
			Pawn->Camera.Get()->AddRelativeRotation(pitch);
		}
		
		//turn BtBody around the UE5 Z axis (so left<->right rotation)
		btQuaternion BodyRot = BulletHelpers::ToBt(FRotator(0, Pawn->GetActorRotation().Yaw, 0));
		Bullet.BtPlayerBodies[PlayerIndex]->getWorldTransform().setRotation(BodyRot);//unsure if this way or motionState() way is deterministic.
	}
}

void GameState::PlayerFire(int PlayerIndex)
{
	if(APlayerPawn* Shooter = (APlayerPawn*)Bullet.BtPlayerBodies[PlayerIndex]->getUserPointer())	{
		btVector3 From = Bullet.BtPlayerBodies[PlayerIndex]->getWorldTransform().getOrigin();
		
		btVector3 Offset = BulletHelpers::ToBtPos(Shooter->Camera->GetRelativeLocation(), FVector(0));
		From += Offset;
		
		btVector3 CamDir = BulletHelpers::ToBtDir(Shooter->Camera->GetForwardVector()) * 10000.f; //prob bigger than necessary 
		btVector3 To = From + CamDir;
		
		btCollisionWorld::ClosestRayResultCallback rayCallback(From, To);
		Bullet.BtWorld->rayTest(From, To, rayCallback);

		//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("Player Fired"));
		DrawDebugLine(Shooter->GetWorld(), BulletHelpers::ToUEPos(From, FVector(0)), BulletHelpers::ToUEPos(To,FVector(0)),FColor::Green, false, 1.f, 0, 1);
		
		if(rayCallback.hasHit())	{
			//Has to be cast this way. Unreal doesn't like turning dumb void* into Unreal UObject*.
			if(APlayerPawn* HitPlayer = (APlayerPawn*)rayCallback.m_collisionObject->getUserPointer())	{
				if (HitPlayer->IsA(APlayerPawn::StaticClass()))	{
					//we hit someone
					PlayerHitsDealt[PlayerIndex]++;
					for(int i = 0; i < NumPlayers; i++)	{
						//we cant shoot ourself
						if(i == PlayerIndex)
							continue;
						//if we hit player i
						if(HitPlayer == Bullet.BtPlayerBodies[i]->getUserPointer())	{
							//they got hit one more time
							PlayerHitsReceived[i]++;
							UE_LOG(LogTemp, Log, TEXT("Player %d Shot %d."), PlayerIndex, i);
							break;
						}
					}
				}
			}
		}
	}
}

int GameState::LoadBtBodyData()
{
	const int TotalBodies = Bullet.BtPlayerBodies.Num() + Bullet.BtRigidBodies.Num();
	if(BtBodyData.Num() == TotalBodies)
	{
		for(int i = 0; i < TotalBodies; i++)	{
			
			if (i < Bullet.BtPlayerBodies.Num())	{
				DeSerializeBtBodyData(Bullet.BtPlayerBodies[i], BtBodyData[i]);
			}
			else	{
				DeSerializeBtBodyData(Bullet.BtRigidBodies[i - Bullet.BtPlayerBodies.Num()], BtBodyData[i]);
			}
		}
	}
	else	{
		//incorrect num of bodies error
		UE_LOG(LogTemp, Error, TEXT("LoadBtBodyData Error: Expected %d Bodies but have Data for %d bodies stored"), TotalBodies, BtBodyData.Num());
		return 0;
	}
	return BtBodyData.Num();
}

int GameState::SaveBtBodyData()
{
	btDefaultSerializer* Serializer = new btDefaultSerializer();
	const int TotalBodies = Bullet.BtPlayerBodies.Num() + Bullet.BtRigidBodies.Num();
	//if missing bodies, do a full save
	if(BtBodyData.Num() < (TotalBodies))	{
		BtBodyData.Empty();
		BtBodyData.Reserve(TotalBodies);
		
		for(int i = 0; i < Bullet.BtPlayerBodies.Num(); i++)
		{
			btRigidBodyFloatData Data;
			Bullet.BtPlayerBodies[i]->serialize(&Data,Serializer);
			BtBodyData.Add(Data);
		}
		for(int i = 0; i < Bullet.BtRigidBodies.Num(); i++)
		{
			btRigidBodyFloatData Data;
			Bullet.BtRigidBodies[i]->serialize(&Data,Serializer);
			BtBodyData.Add(Data);
		}
	}
	//else just update current ones
	else	{
		for(int i = 0; i < TotalBodies; i++)	{
			btRigidBodyFloatData Data;
			
			if (i < Bullet.BtPlayerBodies.Num())	{
				Bullet.BtPlayerBodies[i]->serialize(&Data,Serializer);
			}
			else	{
				Bullet.BtRigidBodies[i - Bullet.BtPlayerBodies.Num()]->serialize(&Data,Serializer);
			}
			BtBodyData[i] = Data;
		}
	}

	if(Serializer)
	{
		delete Serializer;
		Serializer = nullptr;
	}
	return BtBodyData.Num();
}

void GameState::DeSerializeBtBodyData(btRigidBody* OutBody, const btRigidBodyFloatData& InData)
{
	btTransform transform = btTransform(); //used to convert data to usable transform
	btVector3 vec3 = btVector3(); //used to convert data to usable vector
	
	//m_collisionObjectData section
	vec3.deSerializeFloat(InData.m_collisionObjectData.m_anisotropicFriction);
	OutBody->setAnisotropicFriction(vec3, InData.m_collisionObjectData.m_hasAnisotropicFriction);
	OutBody->setContactProcessingThreshold(InData.m_collisionObjectData.m_contactProcessingThreshold);
	OutBody->setDeactivationTime(InData.m_collisionObjectData.m_deactivationTime);
	OutBody->setFriction(InData.m_collisionObjectData.m_friction);
	OutBody->setRollingFriction(InData.m_collisionObjectData.m_rollingFriction);
	OutBody->setContactStiffnessAndDamping(InData.m_collisionObjectData.m_contactStiffness, InData.m_collisionObjectData.m_contactDamping);
	OutBody->setRestitution(InData.m_collisionObjectData.m_restitution);
	OutBody->setHitFraction(InData.m_collisionObjectData.m_hitFraction);
	OutBody->setCcdSweptSphereRadius(InData.m_collisionObjectData.m_ccdSweptSphereRadius);
	OutBody->setCcdMotionThreshold(InData.m_collisionObjectData.m_ccdMotionThreshold);
	OutBody->setCollisionFlags(InData.m_collisionObjectData.m_collisionFlags);
	OutBody->setIslandTag(InData.m_collisionObjectData.m_islandTag1);
	OutBody->setCompanionId(InData.m_collisionObjectData.m_companionId);
	OutBody->setActivationState(InData.m_collisionObjectData.m_activationState1);
	
	//General btRigidBody section
	vec3.deSerializeFloat(InData.m_linearVelocity);
	OutBody->setLinearVelocity(vec3);
	vec3.deSerializeFloat(InData.m_angularVelocity);
	OutBody->setAngularVelocity(vec3);
	vec3.deSerializeFloat(InData.m_angularFactor);
	OutBody->setAngularFactor(vec3);
	vec3.deSerializeFloat(InData.m_linearFactor);
	OutBody->setLinearFactor(vec3);
	OutBody->setDamping(InData.m_linearDamping, InData.m_angularDamping);
	OutBody->setSleepingThresholds(InData.m_linearSleepingThreshold, InData.m_angularSleepingThreshold);

	//doing world transform last seems to fix some determinism problems (as it calls other interal functions that uses other bullet data)
	transform.deSerializeFloat(InData.m_collisionObjectData.m_worldTransform);
	OutBody->setCenterOfMassTransform(transform);
}

