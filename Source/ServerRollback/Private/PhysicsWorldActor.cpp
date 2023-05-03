// Fill out your copyright notice in the Description page of Project Settings.


#include "PhysicsWorldActor.h"
#include "Components/ShapeComponent.h"
#include "Components/CapsuleComponent.h"
#include "PlayerPawn.h"

#include "BulletMain.h"
#include "BulletHelpers.h"
#include "BulletCustomMotionState.h"
#include "BulletDebugDraw.h"

// Sets default values
APhysicsWorldActor::APhysicsWorldActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// This is all pretty standard Bullet bootstrap
	BtCollisionConfig = new btDefaultCollisionConfiguration();
	BtCollisionDispatcher = new btCollisionDispatcher (BtCollisionConfig);
	BtBroadphase = new btDbvtBroadphase ();
	BtConstraintSolver = new btSequentialImpulseConstraintSolver();
	BtWorld = new btDiscreteDynamicsWorld (BtCollisionDispatcher, BtBroadphase, BtConstraintSolver, BtCollisionConfig);

	// I mess with a few settings on BtWorld->getSolverInfo() but they're specific to my needs	

	// Gravity vector in our units (1=1cm)
	BtWorld->setGravity(BulletHelpers::ToBtDir(FVector(0, 0, -980)));


}

// Called when the game starts or when spawned
void APhysicsWorldActor::BeginPlay()
{
	Super::BeginPlay();
	//D option section to enable debug draw mode
	// set up debug rendering
	BtDebugDraw = new BulletDebugDraw(GetWorld(), GetActorLocation());
	BtWorld->setDebugDrawer(BtDebugDraw);
	BtWorld->getDebugDrawer()->setDebugMode(btIDebugDraw::DBG_DrawWireframe);

	SetupStaticGeometryPhysics(PhysicsStaticActors1, PhysicsStatic1Friction, PhysicsStatic1Restitution);

	SetupInitialDynamicPhysics(PhysicsDynamicActors);

}

void APhysicsWorldActor::BeginDestroy()
{
	Super::BeginDestroy();
	if(BtWorld)
	{
		if(BtWorld->getNumCollisionObjects() - 1 >= 0)
		{
			for (int i = BtWorld->getNumCollisionObjects() - 1; i >= 0; i--)
			{
				btCollisionObject* obj = BtWorld->getCollisionObjectArray()[i];
				btRigidBody* body = btRigidBody::upcast(obj);
				if (body && body->getMotionState())
				{
					delete body->getMotionState();
				}
				BtWorld->removeCollisionObject(obj);
				delete obj;
			}
		}
	}
	
	
	// delete collision shapes
	for (int i = 0; i < BtBoxCollisionShapes.Num(); i++)
		delete BtBoxCollisionShapes[i];
	BtBoxCollisionShapes.Empty();
	for (int i = 0; i < BtSphereCollisionShapes.Num(); i++)
		delete BtSphereCollisionShapes[i];
	BtSphereCollisionShapes.Empty();
	for (int i = 0; i < BtCapsuleCollisionShapes.Num(); i++)
		delete BtCapsuleCollisionShapes[i];
	BtCapsuleCollisionShapes.Empty();
	for (int i = 0; i < BtConvexHullCollisionShapes.Num(); i++)
		delete BtConvexHullCollisionShapes[i].Shape;
	BtConvexHullCollisionShapes.Empty();
	for (int i = 0; i < CachedDynamicShapes.Num(); i++)
	{
		// Only delete if this is a compound shape, otherwise it's an alias to other simple arrays
		if (CachedDynamicShapes[i].bIsCompound)
			delete CachedDynamicShapes[i].Shape;
	}
	CachedDynamicShapes.Empty();

	delete BtWorld;
	delete BtConstraintSolver;
	delete BtBroadphase;
	delete BtCollisionDispatcher;
	delete BtCollisionConfig;
	delete BtDebugDraw; // I haven't talked about this yet, later

	BtWorld = nullptr;
	BtConstraintSolver = nullptr;
	BtBroadphase = nullptr;
	BtCollisionDispatcher = nullptr;
	BtCollisionConfig = nullptr;
	BtDebugDraw = nullptr;

	// Clear our type-specific arrays (duplicate refs)
	BtStaticObjects.Empty();
	BtRigidBodies.Empty();
}

// Called every frame
void APhysicsWorldActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	StepPhysics(DeltaTime);
}

void APhysicsWorldActor::UpdatePlayerPhysics(APlayerPawn* Pawn)
{
	FVector InputVec = Pawn->ConsumeMovementInputVector();

	for(btRigidBody* Body : BtRigidBodies)
	{
		if(Pawn == Body->getUserPointer())
		{
			/*Body->setCollisionFlags(Body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			Body->setActivationState(DISABLE_DEACTIVATION);*/
			//Body->applyCentralImpulse(BulletHelpers::ToBtDir(InputVec * 10.f, false));
			Body->setLinearVelocity(BulletHelpers::ToBtDir(InputVec * 10.f, false));
			if(GEngine)
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("applied force to player"));
		}
	}
	
}

void APhysicsWorldActor::StepPhysics(float DeltaSeconds)
{
	//D stepSimulation() returns number of simulation sub steps. I'll need to do this externally in ggpo
	TimeStepAccumulator += DeltaSeconds;
	//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Emerald, FString::Printf(TEXT("AccuTime: %f || DeltaTime = %f"), TimeStepAccumulator, DeltaSeconds));
	int steps = 0;
	while(TimeStepAccumulator >= BtPhysicsTimeStep)
	{
		BtWorld->stepSimulation(BtPhysicsTimeStep, BtMaxSubSteps, BtPhysicsTimeStep);
		TimeStepAccumulator -= BtPhysicsTimeStep;
		steps++;
	}
	//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Emerald, FString::Printf(TEXT("Num Steps: %d"), steps));


#if WITH_EDITORONLY_DATA
	if (bPhysicsShowDebug)
	{
		BtWorld->debugDrawWorld();
		//BtDebugDraw->drawLine(btVector3(0.0, 0.0, 0.0), btVector3(100.0f, 100.0f, 100.0f), btVector3(255,0,0));

		//DrawDebugSphere(GetWorld(), BulletHelpers::ToUEPos(BtRigidBodies.Last()->getWorldTransform().getOrigin(), GetActorLocation()), 50.0f, 4, FColor::Red, false, 1, 0, 2);
	}
#endif
	
}

void APhysicsWorldActor::SetupStaticGeometryPhysics(TArray<AActor*> Actors, float Friction, float Restitution)
{
	for (AActor* Actor : Actors)
	{
		// Just in case we remove items from the list & leave blank
		if (Actor == nullptr)
			continue;

		ExtractPhysicsGeometry(Actor, [Actor, this, Friction, Restitution](btCollisionShape* Shape, const FTransform& RelTransform)
			{
				// Every sub-collider in the actor is passed to this callback function
				// We're baking this in world space, so apply actor transform to relative
				const FTransform FinalXform = RelTransform * Actor->GetActorTransform();
				AddStaticCollision(Shape, FinalXform, Friction, Restitution, Actor);
			});
		UE_LOG(LogTemp, Warning, TEXT("Static Actor: %s was added. Shape is %hs"), *Actor->GetName(), BtStaticObjects.Last()->getCollisionShape()->getName());
	}
}

void APhysicsWorldActor::SetupInitialDynamicPhysics(TArray<AActor*> Actors)
{
	for (AActor* Actor : Actors)
	{
		// Just in case we remove items from the list & leave blank
		if (Actor == nullptr)
			continue;
		
		const UPrimitiveComponent* Component = Actor->FindComponentByClass<UPrimitiveComponent>();
		if(Component != nullptr)
		{
			/*AddRigidBody(Actor, GetCachedDynamicShapeData(Actor, Component->GetMass()));
			UE_LOG(LogTemp, Warning, TEXT("Actor: %s has a mass of %f from %s"), *Actor->GetName(), Component->GetMass(), *Component->GetName());*/
			
			//D Using default mass for now, to avoid warning message
			AddRigidBody(Actor, GetCachedDynamicShapeData(Actor, 100.f));
			UE_LOG(LogTemp, Warning, TEXT("Actor: %s has a mass of %f from %s"), *Actor->GetName(), 100.f, *Component->GetName());
		
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Actor: %s has no UPrimitiveComponent, using default mass of 100"), *Actor->GetName());
			AddRigidBody(Actor, GetCachedDynamicShapeData(Actor, 100.f));
		}
		UE_LOG(LogTemp, Warning, TEXT("Initial Dynamic Actor: %s was added"), *Actor->GetName());
	}
	
}

btRigidBody* APhysicsWorldActor::AddPhysicsPlayer(APlayerPawn* Pawn)
{
	// Just in case we remove items from the list & leave blank
	if (Pawn == nullptr)
		return nullptr;
	
	const UCapsuleComponent* Capsule = Pawn->GetCapsuleComponent();
	if(Capsule != nullptr)
	{
		//D Create bullet capsule
		CachedDynamicShapeData CapsuleData;		
		//D Find already cached shape if exists.
		const FName ClassName = Pawn->GetClass()->GetFName();
		for (auto&& Data: CachedDynamicShapes)
		{
			if (Data.ClassName == ClassName)
			{
				CapsuleData = Data;
				break;
			}
		}
		//if not found (hasnt been cached yet)
		if(CapsuleData.ClassName == FName())
		{
			btCollisionShape* Shape = GetCapsuleCollisionShape(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight()-2.f);
			/*// Capsules are in Z in UE, in Y in Bullet, so roll -90
			FRotator Rot(0, 0, 0); // Rot(0, 0, -90)
			// Also apply any local rotation
			Rot += Capsule->GetRelativeRotation();
			FTransform ShapeXform(Rot, Capsule->GetRelativeLocation());
			// Shape transform adds to any relative transform already here
			FTransform XForm = ShapeXform * (Capsule->GetComponentTransform() * Pawn->GetActorTransform().Inverse());*/

			//D Player capsule shape data
			CapsuleData.ClassName = Pawn->GetClass()->GetFName();
			CapsuleData.Shape = Shape;
			CapsuleData.bIsCompound = false;
			CapsuleData.Mass = 100.f;
			CapsuleData.Shape->calculateLocalInertia(CapsuleData.Mass, CapsuleData.Inertia);
			CachedDynamicShapes.Add(CapsuleData);
		}
		UE_LOG(LogTemp, Warning, TEXT("Pawn: %s was added"), *Pawn->GetName());
		return AddPlayerBody(Pawn, CapsuleData);
	}
	else
		return nullptr;
}

btCollisionObject* APhysicsWorldActor::AddStaticCollision(btCollisionShape* Shape, const FTransform& Transform, float Friction, float Restitution, AActor* Actor)
{
	btTransform Xform = BulletHelpers::ToBt(Transform, GetActorLocation());
	btCollisionObject* Obj = new btCollisionObject();
	Obj->setCollisionShape(Shape);
	Obj->setWorldTransform(Xform);
	Obj->setFriction(Friction);
	Obj->setRestitution(Restitution);
	Obj->setUserPointer(Actor);
	BtWorld->addCollisionObject(Obj);
	BtStaticObjects.Add(Obj);
	return Obj;
}

void APhysicsWorldActor::ExtractPhysicsGeometry(AActor* Actor, PhysicsGeometryCallback CB)
{
	TInlineComponentArray<UActorComponent*, 20> Components;
	// Used to easily get a component's transform relative to actor, not parent component
	const FTransform InvActorTransform = Actor->GetActorTransform().Inverse();

	// Collisions from meshes
	Actor->GetComponents(UStaticMeshComponent::StaticClass(), Components);
	for (auto && Comp : Components)
	{
		ExtractPhysicsGeometry(Cast<UStaticMeshComponent>(Comp), InvActorTransform, CB);
	}
	// Collisions from separate collision components
	Actor->GetComponents(UShapeComponent::StaticClass(), Components);
	for (auto && Comp : Components)
	{
		ExtractPhysicsGeometry(Cast<UShapeComponent>(Comp), InvActorTransform, CB);
	}	
}

void APhysicsWorldActor::ExtractPhysicsGeometry(UStaticMeshComponent* SMC, const FTransform& InvActorXform, PhysicsGeometryCallback CB)
{
	UStaticMesh* Mesh = SMC->GetStaticMesh();
	if (!Mesh)
		return;

	//D if component is part of an actor, get its actor+component transform
	if(SMC != SMC->GetAttachmentRoot())
	{
		// We want the complete transform from actor to this component, not just relative to parent
		FTransform CompFullRelXForm =  SMC->GetComponentTransform() * InvActorXform;
		ExtractPhysicsGeometry(CompFullRelXForm, Mesh->GetBodySetup(), CB);
	}
	//D if component is THE root/actor, get its transform directly and re-scale it (since scale gets negated otherwise)
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Component: %s is root"), *SMC->GetName());
		FTransform CompFullRelXForm =  SMC->GetComponentTransform() * InvActorXform;
		CompFullRelXForm.SetScale3D(SMC->GetComponentScale());//D re-scale it
		ExtractPhysicsGeometry(CompFullRelXForm, Mesh->GetBodySetup(), CB);
	}
	

	// Not supporting complex collision shapes right now
	// If we did, note that Mesh->ComplexCollisionMesh is WITH_EDITORONLY_DATA so not available at runtime
	// See StaticMeshRender.cpp, FStaticMeshSceneProxy::GetDynamicMeshElements
	// Line 1417+, bDrawComplexCollision
	// Looks like we have to access LODForCollision, RenderData->LODResources
	// So they use a mesh LOD for collision for complex shapes, never drawn usually?

}

void APhysicsWorldActor::ExtractPhysicsGeometry(UShapeComponent* Sc, const FTransform& InvActorXform, PhysicsGeometryCallback CB)
{
	// We want the complete transform from actor to this component, not just relative to parent
	FTransform CompFullRelXForm =  Sc->GetComponentTransform() * InvActorXform;
	ExtractPhysicsGeometry(CompFullRelXForm, Sc->ShapeBodySetup, CB);	
}


void APhysicsWorldActor::ExtractPhysicsGeometry(const FTransform& XformSoFar, UBodySetup* BodySetup, PhysicsGeometryCallback CB)
{
	FVector Scale = XformSoFar.GetScale3D();
	btCollisionShape* Shape = nullptr;

	UE_LOG(LogTemp, Warning, TEXT("Scale: %s"), *Scale.ToString());

	// Iterate over the simple collision shapes
	for (auto && Box : BodySetup->AggGeom.BoxElems)
	{
		// We'll re-use based on just the LxWxH, including actor scale
		// Rotation and centre will be baked in world space
		FVector Dimensions = FVector(Box.X, Box.Y, Box.Z) * Scale;
		Shape = GetBoxCollisionShape(Dimensions);
		FTransform ShapeXform(Box.Rotation, Box.Center);
		// Shape transform adds to any relative transform already here
		FTransform XForm = ShapeXform * XformSoFar;
		CB(Shape, XForm);
	}
	for (auto && Sphere : BodySetup->AggGeom.SphereElems)
	{
		// Only support uniform scale so use X
		Shape = GetSphereCollisionShape(Sphere.Radius * Scale.X);
		FTransform ShapeXform(FRotator::ZeroRotator, Sphere.Center);
		// Shape transform adds to any relative transform already here
		FTransform XForm = ShapeXform * XformSoFar;
		CB(Shape, XForm);
	}
	// Sphyl == Capsule (??)
	for (auto && Capsule : BodySetup->AggGeom.SphylElems)
	{
		// X scales radius, Z scales height
		Shape = GetCapsuleCollisionShape(Capsule.Radius * Scale.X, Capsule.Length * Scale.Z);
		// Capsules are in Z in UE, in Y in Bullet, so roll -90
		//D ^ nevermind this, changed it to fix that elswhere using btCapsuleShapeZ
		// FRotator Rot(0, 0, -90);
		// // Also apply any local rotation
		// Rot += Capsule.Rotation;
		FTransform ShapeXform(Capsule.Rotation, Capsule.Center); //D was (Rot, Capsule.center)
		// Shape transform adds to any relative transform already here
		FTransform XForm = ShapeXform * XformSoFar;
		CB(Shape, XForm);
	}
	for (int i = 0; i < BodySetup->AggGeom.ConvexElems.Num(); ++i)
	{
		Shape = GetConvexHullCollisionShape(BodySetup, i, Scale);
		CB(Shape, XformSoFar);
	}
	
}

btCollisionShape* APhysicsWorldActor::GetBoxCollisionShape(const FVector& Dimensions)
{
	// Simple brute force lookup for now, probably doesn't need anything more clever
	btVector3 HalfSize = BulletHelpers::ToBtSize(Dimensions * 0.5);
	for (auto && S : BtBoxCollisionShapes)
	{
		btVector3 Sz = S->getHalfExtentsWithMargin();
		if (FMath::IsNearlyEqual(Sz.x(), HalfSize.x()) && 
			FMath::IsNearlyEqual(Sz.y(), HalfSize.y()) &&
			FMath::IsNearlyEqual(Sz.z(), HalfSize.z()))
		{
			return S;
		}
	}

	// Not found, create
	auto S = new btBoxShape(HalfSize);
	// Get rid of margins, just cause issues for me
	S->setMargin(0);
	BtBoxCollisionShapes.Add(S);

	return S;
	
}

btCollisionShape* APhysicsWorldActor::GetSphereCollisionShape(float Radius)
{
	// Simple brute force lookup for now, probably doesn't need anything more clever
	btScalar Rad = BulletHelpers::ToBtSize(Radius);
	for (auto && S : BtSphereCollisionShapes)
	{
		// Bullet subtracts a margin from its internal shape, so add back to compare
		if (FMath::IsNearlyEqual(S->getRadius(), Rad))
		{
			return S;
		}
	}

	// Not found, create
	auto S = new btSphereShape(Rad);
	// Get rid of margins, just cause issues for me
	S->setMargin(0);
	BtSphereCollisionShapes.Add(S);

	return S;
	
}

btCollisionShape* APhysicsWorldActor::GetCapsuleCollisionShape(float Radius, float Height)
{
	// Simple brute force lookup for now, probably doesn't need anything more clever
	btScalar R = BulletHelpers::ToBtSize(Radius);
	btScalar H = BulletHelpers::ToBtSize(Height);
	btScalar HalfH = H * 0.5f;
	
	for (auto && S : BtCapsuleCollisionShapes)
	{
		// Bullet subtracts a margin from its internal shape, so add back to compare
		if (FMath::IsNearlyEqual(S->getRadius(), R) &&
			FMath::IsNearlyEqual(S->getHalfHeight(), HalfH))
		{
			return S;
		}
	}

	// Not found, create
	auto S = new btCapsuleShapeZ(R, H); //D changed from btCapsuleShape to btCapsuleShapeZ so that its upright in Unreal coordinates
	BtCapsuleCollisionShapes.Add(S);

	return S;
	
}

btCollisionShape* APhysicsWorldActor::GetConvexHullCollisionShape(UBodySetup* BodySetup, int ConvexIndex, const FVector& Scale)
{
	//D added Bt to start of name ConvexHullCollisionShapes
	UE_LOG(LogTemp, Warning, TEXT("Scale: %s"), *Scale.ToString());

	
	for (auto && S : BtConvexHullCollisionShapes)
	{
		if (S.BodySetup == BodySetup && S.HullIndex == ConvexIndex && S.Scale.Equals(Scale))
		{
			return S.Shape;
		}
	}
	
	const FKConvexElem& Elem = BodySetup->AggGeom.ConvexElems[ConvexIndex];
	auto C = new btConvexHullShape();
	//D fixed component scale not being applied to Convex shapes
	C->setLocalScaling(btVector3(Scale.X,Scale.Y,Scale.Z));
	
	for (auto && P : Elem.VertexData)
	{
		C->addPoint(BulletHelpers::ToBtPos(P, FVector::ZeroVector));
	}
	// Very important! Otherwise there's a gap between 
	C->setMargin(0);
	// Apparently this is good to call?
	C->initializePolyhedralFeatures();
	
	
	BtConvexHullCollisionShapes.Add({
        BodySetup,
		ConvexIndex,
		Scale,
		C
    });

	return C;
}

const APhysicsWorldActor::CachedDynamicShapeData& APhysicsWorldActor::GetCachedDynamicShapeData(AActor* Actor, float Mass)
{
	// We re-use compound shapes based on (leaf) BP class
	const FName ClassName = Actor->GetClass()->GetFName();
	for (auto&& Data: CachedDynamicShapes)
	{
		if (Data.ClassName == ClassName)
			return Data;
	}

	// Because we want to support compound colliders, we need to extract all colliders first before
	// constructing the final body.
	TArray<btCollisionShape*, TInlineAllocator<20>> Shapes;
	TArray<FTransform, TInlineAllocator<20>> ShapeRelXforms;
	ExtractPhysicsGeometry(Actor,
		[&Shapes, &ShapeRelXforms](btCollisionShape* Shape, const FTransform& RelTransform)
		{
			Shapes.Add(Shape);
			ShapeRelXforms.Add(RelTransform);
		});


	CachedDynamicShapeData ShapeData;
	ShapeData.ClassName = ClassName;
	
	// Single shape with no transform is simplest
	if (ShapeRelXforms.Num() == 1 &&
		ShapeRelXforms[0].EqualsNoScale(FTransform::Identity))
	{
		ShapeData.Shape = Shapes[0];
		// just to make sure we don't think we have to clean it up; simple shapes are already stored
		ShapeData.bIsCompound = false; 
	}
	else
	{
		// Compound or offset single shape; we will cache these by blueprint type
		btCompoundShape* CS = new btCompoundShape();
		for (int i = 0; i < Shapes.Num(); ++i)
		{
			// We don't use the actor origin when converting transform in this case since object space
			// Note that btCompoundShape doesn't free child shapes, which is fine since they're tracked separately
			CS->addChildShape(BulletHelpers::ToBt(ShapeRelXforms[i], FVector::ZeroVector), Shapes[i]);
		}

		ShapeData.Shape = CS;
		ShapeData.bIsCompound = true;
	}

	// Calculate Inertia
	ShapeData.Mass = Mass;
	ShapeData.Shape->calculateLocalInertia(Mass, ShapeData.Inertia);

	// Cache for future use
	CachedDynamicShapes.Add(ShapeData);

	return CachedDynamicShapes.Last();
	
}

btRigidBody* APhysicsWorldActor::AddRigidBody(AActor* Actor, const CachedDynamicShapeData& ShapeData)
{
	return AddRigidBody(Actor, ShapeData.Shape, ShapeData.Inertia, ShapeData.Mass);
}

btRigidBody* APhysicsWorldActor::AddRigidBody(AActor* Actor, btCollisionShape* CollisionShape, btVector3 Inertia, float Mass)
{
	auto Origin = GetActorLocation();
	auto MotionState = new BulletCustomMotionState(Actor, Origin);
	const btRigidBody::btRigidBodyConstructionInfo rbInfo(Mass, MotionState, CollisionShape, Inertia);
	btRigidBody* Body = new btRigidBody(rbInfo);
	Body->setUserPointer(Actor);
	BtWorld->addRigidBody(Body);
	BtRigidBodies.Add(Body);

	return Body;
}
//D Add the Unreal Player Pawn added to the Bullet3 Physics Simulation World for Deterministic Gameplay.
btRigidBody* APhysicsWorldActor::AddPlayerBody(APlayerPawn* Pawn, const CachedDynamicShapeData& ShapeData)
{
	auto Origin = GetActorLocation();
	auto MotionState = new BulletCustomMotionState(Pawn, Origin);
	const btRigidBody::btRigidBodyConstructionInfo rbInfo(ShapeData.Mass, MotionState, ShapeData.Shape, ShapeData.Inertia);
	btRigidBody* Body = new btRigidBody(rbInfo);
	
	Body->setAngularFactor(0.f);
	Body->setActivationState(DISABLE_DEACTIVATION);
	
	Body->setUserPointer(Pawn);
	BtWorld->addRigidBody(Body);
	BtRigidBodies.Add(Body);

	return Body;
}
