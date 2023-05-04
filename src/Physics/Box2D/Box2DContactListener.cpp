#include "Physics/Box2D/Box2DContactListener.h"

#include <box2d/b2_contact.h>

namespace puffin::physics
{
	void Box2DContactListener::BeginContact(b2Contact* contact)
	{
		CollisionBeginEvent collisionBeginEvent;
		collisionBeginEvent.entityA = static_cast<ECS::EntityID>(contact->GetFixtureA()->GetBody()->GetUserData().pointer);
		collisionBeginEvent.entityB = static_cast<ECS::EntityID>(contact->GetFixtureB()->GetBody()->GetUserData().pointer);

		collisionBeginEvents_.Push(collisionBeginEvent);
	}

	void Box2DContactListener::EndContact(b2Contact* contact)
	{
		CollisionEndEvent collisionEndEvent;
		collisionEndEvent.entityA = static_cast<ECS::EntityID>(contact->GetFixtureA()->GetBody()->GetUserData().pointer);
		collisionEndEvent.entityB = static_cast<ECS::EntityID>(contact->GetFixtureB()->GetBody()->GetUserData().pointer);

		collisionEndEvents_.Push(collisionEndEvent);
	}

	void Box2DContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
	{
		
	}

	void Box2DContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
	{
		
	}

	bool Box2DContactListener::getNextCollisionBeginEvent(CollisionBeginEvent& collisionBeginEvent)
	{
		return collisionBeginEvents_.Pop(collisionBeginEvent);
	}

	bool Box2DContactListener::getNextCollisionEndEvent(CollisionEndEvent& collisionEndEvent)
	{
		return collisionEndEvents_.Pop(collisionEndEvent);
	}

	void Box2DContactListener::clearEvents()
	{
		collisionBeginEvents_.Flush();
		collisionEndEvents_.Flush();
	}
}
