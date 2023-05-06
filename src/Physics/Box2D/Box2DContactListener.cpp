#include "Physics/Box2D/Box2DContactListener.h"

#include <box2d/b2_contact.h>

namespace puffin::physics
{
	void Box2DContactListener::BeginContact(b2Contact* contact)
	{
		CollisionBeginEvent collisionBeginEvent;
		collisionBeginEvent.entityA = static_cast<PuffinID>(contact->GetFixtureA()->GetBody()->GetUserData().pointer);
		collisionBeginEvent.entityB = static_cast<PuffinID>(contact->GetFixtureB()->GetBody()->GetUserData().pointer);

		mCollisionBeginEvents.push(collisionBeginEvent);
	}

	void Box2DContactListener::EndContact(b2Contact* contact)
	{
		CollisionEndEvent collisionEndEvent;
		collisionEndEvent.entityA = static_cast<PuffinID>(contact->GetFixtureA()->GetBody()->GetUserData().pointer);
		collisionEndEvent.entityB = static_cast<PuffinID>(contact->GetFixtureB()->GetBody()->GetUserData().pointer);

		mCollisionEndEvents.push(collisionEndEvent);
	}

	void Box2DContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
	{
		
	}

	void Box2DContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
	{
		
	}

	bool Box2DContactListener::getNextCollisionBeginEvent(CollisionBeginEvent& collisionBeginEvent)
	{
		return mCollisionBeginEvents.pop(collisionBeginEvent);
	}

	bool Box2DContactListener::getNextCollisionEndEvent(CollisionEndEvent& collisionEndEvent)
	{
		return mCollisionEndEvents.pop(collisionEndEvent);
	}

	void Box2DContactListener::clearEvents()
	{
		mCollisionBeginEvents.flush();
		mCollisionEndEvents.flush();
	}
}
