// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "BulletMinimal.h"
#include "PhysicsWorldActor.generated.h"

class BulletDebugDraw;
class btDiscreteDynamicsWorld;
class APlayerPawn;
//Class is sourced from (link), I have extended the functionality. https://www.stevestreeting.com/2020/07/26/using-bullet-for-physics-in-ue4/
UCLASS()
class SERVERROLLBACK_API APhysicsWorldActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APhysicsWorldActor();

	virtual void BeginDestroy() override;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void UpdatePlayerPhysics(APlayerPawn* Pawn);

	//D Static Actor Properties section
	
	// This list can be edited in the level, linking to placed static actors
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bullet Physics|Objects")
	TArray<AActor*> PhysicsStaticActors1;
	// These properties can only be edited in the Blueprint
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Bullet Physics|Objects")
	float PhysicsStatic1Friction = 0.6;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Bullet Physics|Objects")
	float PhysicsStatic1Restitution = 0.3;

	//D I added this
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bullet Physics|Objects")
	TArray<AActor*> PhysicsDynamicActors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bullet Physics|Debug")
	bool bPhysicsShowDebug = false;
	// Custom debug interface
	BulletDebugDraw* BtDebugDraw;
private:
	//D Bullet default for maxSubsteps is 3, using 0 enables manual sub stepping
	int BtMaxSubSteps = 0;
	// 1 / desiredFPS (default 60)
	double BtPhysicsTimeStep = 1./60.0; 
	btScalar TimeStepAccumulator = 0;
	
	void StepPhysics(float DeltaSeconds);
	//D Data storage section
	//...

	// Bullet section
	// Global objects
	btCollisionConfiguration* BtCollisionConfig;
	btCollisionDispatcher* BtCollisionDispatcher;
	btBroadphaseInterface* BtBroadphase;
	btConstraintSolver* BtConstraintSolver;
	btDiscreteDynamicsWorld* BtWorld;

	// Dynamic bodies
	TArray<btRigidBody*> BtRigidBodies;
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

	//...
	//D data storage section end

	//D Static Collision section
	void SetupStaticGeometryPhysics(TArray<AActor*> Actors, float Friction, float Restitution);

	//D I added
	void SetupInitialDynamicPhysics(TArray<AActor*> Actors);
public:
	//D Add player pawn to bullet, return with player index
	btRigidBody* AddPhysicsPlayer(APlayerPawn* Pawn);
	btDiscreteDynamicsWorld* GetBtWorld() const { return BtWorld; }
	
private:
	btCollisionObject* AddStaticCollision(btCollisionShape* Shape, const FTransform& Transform, float Friction, float Restitution, AActor* Actor);

	typedef const std::function<void (btCollisionShape* /*SingleShape*/, const FTransform& /*RelativeXform*/)>& PhysicsGeometryCallback;
	void ExtractPhysicsGeometry(AActor* Actor, PhysicsGeometryCallback CB);
	void ExtractPhysicsGeometry(UStaticMeshComponent* SMC, const FTransform& InvActorXform, PhysicsGeometryCallback CB);
	void ExtractPhysicsGeometry(UShapeComponent* Sc, const FTransform& InvActorXform, PhysicsGeometryCallback CB);
	void ExtractPhysicsGeometry(const FTransform& XformSoFar, UBodySetup* BodySetup, PhysicsGeometryCallback CB);

	btCollisionShape* GetBoxCollisionShape(const FVector& Dimensions);
	btCollisionShape* GetSphereCollisionShape(float Radius);
	btCollisionShape* GetCapsuleCollisionShape(float Radius, float Height);
	btCollisionShape* GetConvexHullCollisionShape(UBodySetup* BodySetup, int ConvexIndex, const FVector& Scale);

	//D Static Collision section end

	//D Dynamic collision
	const CachedDynamicShapeData& GetCachedDynamicShapeData(AActor* Actor, float Mass);

	btRigidBody* AddRigidBody(AActor* Actor, const CachedDynamicShapeData& ShapeData);
	btRigidBody* AddRigidBody(AActor* Actor, btCollisionShape* CollisionShape, btVector3 Inertia, float Mass);
	//D
	btRigidBody* AddPlayerBody(APlayerPawn* Pawn, const CachedDynamicShapeData& ShapeData);
	
};
