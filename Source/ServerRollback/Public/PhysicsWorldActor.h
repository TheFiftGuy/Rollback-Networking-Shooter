// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GGPOGame/GGPOGame.h"
#include "GameFramework/Actor.h"
#include "PhysicsWorldActor.generated.h"

class BulletDebugDraw;
class APlayerPawn;

//The Setup and general structure of Bullet in Unreal is done according to this source :
//https://www.stevestreeting.com/2020/07/26/using-bullet-for-physics-in-ue4/
//Alot of the data storage/pointers have been moved to the (external) GameState.h, but functions still remain here since they use UE.
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

	void InitPhysWorld();

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
	
	//D reference to the UE GameState
	UPROPERTY()
	class AGGPOGameStateBase* GGPOGameStateBase;
private:
	
	void SetupStaticGeometryPhysics(TArray<AActor*> Actors, float Friction, float Restitution);

	//D I added
	void SetupInitialDynamicPhysics(TArray<AActor*> Actors);
public:
	//D Add player pawn to bullet, return with player index
	btRigidBody* AddPhysicsPlayer(APlayerPawn* Pawn);
	//B btDiscreteDynamicsWorld* GetBtWorld() const { return BtWorld; }
	

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
	const BulletPhysics::CachedDynamicShapeData& GetCachedDynamicShapeData(AActor* Actor, float Mass);

	btRigidBody* AddRigidBody(AActor* Actor, const BulletPhysics::CachedDynamicShapeData& ShapeData);
	btRigidBody* AddRigidBody(AActor* Actor, btCollisionShape* CollisionShape, btVector3 Inertia, float Mass);
	//D
	btRigidBody* AddPlayerBody(APlayerPawn* Pawn, const BulletPhysics::CachedDynamicShapeData& ShapeData);
	
};
