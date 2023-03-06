#pragma once

#include "BulletMinimal.h"
#include "BulletHelpers.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

/**
 * Customised MotionState which propagates motion to linked Actor & tracks when sleeping
 */

//Class originates from https://www.stevestreeting.com/2020/07/26/using-bullet-for-physics-in-ue4/
class SERVERROLLBACK_API BulletCustomMotionState : public btMotionState
{
protected:
	TWeakObjectPtr<AActor> Parent;
	// Bullet is made local so that all sims are close to origin
	// This world origin must be in *UE dimensions*
	FVector WorldOrigin; 
	btTransform CenterOfMassTransform;


public:
	BulletCustomMotionState()
	{
		
	}
	BulletCustomMotionState(AActor* ParentActor, const FVector& WorldCentre, const btTransform& CenterOfMassOffset = btTransform::getIdentity())
		: Parent(ParentActor), WorldOrigin(WorldCentre), CenterOfMassTransform(CenterOfMassOffset)

	{
	}

	///synchronizes world transform from UE to physics (typically only called at start)
	void getWorldTransform(btTransform& OutCenterOfMassWorldTrans) const override
	{
		if (Parent.IsValid())
		{
			auto&& Xform = Parent->GetActorTransform();
			OutCenterOfMassWorldTrans = BulletHelpers::ToBt(Parent->GetActorTransform(), WorldOrigin) * CenterOfMassTransform.inverse();
		}
	
	}

	///synchronizes world transform from physics to UE
	void setWorldTransform(const btTransform& CenterOfMassWorldTrans) override
	{
		// send this to actor
		if (Parent.IsValid(false))
		{
			btTransform GraphicTrans = CenterOfMassWorldTrans * CenterOfMassTransform;
			Parent->SetActorTransform(BulletHelpers::ToUE(GraphicTrans, WorldOrigin));
		}
	}
};