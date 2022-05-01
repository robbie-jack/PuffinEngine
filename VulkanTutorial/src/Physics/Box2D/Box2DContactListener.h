#pragma once

#include "box2d/b2_world_callbacks.h"
#include "CollisionEvent.h"
#include "Types/RingBuffer.h"

namespace Puffin::Physics
{
	class Box2DContactListener : public b2ContactListener
	{
	public:

		Box2DContactListener() = default;
		~Box2DContactListener() override
		{
			m_collisionBeginEvents.Flush();
			m_collisionEndEvents.Flush();
		}

		void BeginContact(b2Contact* contact) override;
		void EndContact(b2Contact* contact) override;
		void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override;
		void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override;

		bool GetNextCollisionBeginEvent(CollisionBeginEvent& collisionBeginEvent);
		bool GetNextCollisionEndEvent(CollisionEndEvent& collisionEndEvent);

		void ClearEvents();

	private:

		RingBuffer<CollisionBeginEvent> m_collisionBeginEvents;
		RingBuffer<CollisionEndEvent> m_collisionEndEvents;

	};
}