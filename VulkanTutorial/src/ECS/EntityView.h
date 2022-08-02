#pragma once

#include "Entity.h"
#include "ComponentType.h"

#include <memory>
#include <set>

#include "ECS.h"

namespace Puffin::ECS
{
	class World;

	template<typename... ComponentTypes>
	class EntityView
	{
	public:

		EntityView(std::shared_ptr<World> world)
		{
			m_world = world;

			if (sizeof...(ComponentTypes) == 0)
			{
				m_allEntities = true;
			}
			else
			{
				Init();
			}
		}

		class Iterator
		{
		public:

			Iterator(std::shared_ptr<World> world, std::set<Entity>::iterator iterator, Signature signature, bool allEntities)
				: m_world(world), m_entitiesIterator(iterator), m_signature(signature), m_allEntities(allEntities) {}

			// Return entity that we're currently at
			Entity operator*() const
			{
				return *m_entitiesIterator;
			}

			// Compare two iterators
			bool operator==(const Iterator& other) const
			{
				return (*m_entitiesIterator) == *other;
			}

			// Compare two iterators
			bool operator!=(const Iterator& other) const
			{
				return *m_entitiesIterator != *other;
			}

			// Move iterator forward
			Iterator& operator++()
			{
				do
				{
					++m_entitiesIterator;
				} while (m_entitiesIterator != m_world->GetActiveEntities().end() && !ValidEntity());

				return *this;
			}

		private:

			bool ValidEntity() const
			{
				return m_allEntities || (m_world->GetEntitySignature(*m_entitiesIterator) & m_signature) == m_signature;
			}

			std::shared_ptr<World> m_world = nullptr;
			std::set<Entity>::iterator m_entitiesIterator;

			Signature m_signature;
			bool m_allEntities = false;
		};

		// Return an iterator to beginning of this view
		Iterator begin() const
		{
			auto iterator = m_world->GetActiveEntities().begin();

			while (m_signature != (m_signature & m_world->GetEntitySignature(*iterator)))
			{
				++iterator;
			}

			return Iterator(m_world, iterator, m_signature, m_allEntities);
		}

		// Return an iterator to end of this view
		Iterator end() const
		{
			auto iterator = m_world->GetActiveEntities().end();

			return Iterator(m_world, iterator, m_signature, m_allEntities);
		}

	private:

		void Init()
		{
			assert(m_world != nullptr && "World pointer is null");

			//Unpack component types into initializer list
			ComponentType componentTypes[] = { m_world->GetComponentType<ComponentTypes>() ... };

			// Iterate over component types, setting bit for each in signature
			for (int i = 0; i < sizeof...(ComponentTypes); i++)
			{
				m_signature.set(componentTypes[i]);
			}
		}

		std::shared_ptr<World> m_world = nullptr;
		Signature m_signature;
		bool m_allEntities = false;

	};
}
