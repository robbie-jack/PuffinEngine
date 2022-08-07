#include "Box2DContactListener.h"

#include <box2d/b2_contact.h>

namespace Puffin::Physics
{
	void Box2DContactListener::BeginContact(b2Contact* contact)
	{
		CollisionBeginEvent collisionBeginEvent;
		collisionBeginEvent.entityA = static_cast<ECS::EntityID>(contact->GetFixtureA()->GetBody()->GetUserData().pointer);
		collisionBeginEvent.entityB = static_cast<ECS::EntityID>(contact->GetFixtureB()->GetBody()->GetUserData().pointer);

		m_collisionBeginEvents.Push(collisionBeginEvent);
	}

	void Box2DContactListener::EndContact(b2Contact* contact)
	{
		CollisionEndEvent collisionEndEvent;
		collisionEndEvent.entityA = static_cast<ECS::EntityID>(contact->GetFixtureA()->GetBody()->GetUserData().pointer);
		collisionEndEvent.entityB = static_cast<ECS::EntityID>(contact->GetFixtureB()->GetBody()->GetUserData().pointer);

		m_collisionEndEvents.Push(collisionEndEvent);
	}

	void Box2DContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
	{
		
	}

	void Box2DContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
	{
		
	}

	bool Box2DContactListener::GetNextCollisionBeginEvent(CollisionBeginEvent& collisionBeginEvent)
	{
		return m_collisionBeginEvents.Pop(collisionBeginEvent);
	}

	bool Box2DContactListener::GetNextCollisionEndEvent(CollisionEndEvent& collisionEndEvent)
	{
		return m_collisionEndEvents.Pop(collisionEndEvent);
	}

	void Box2DContactListener::ClearEvents()
	{
		m_collisionBeginEvents.Flush();
		m_collisionEndEvents.Flush();
	}
}
