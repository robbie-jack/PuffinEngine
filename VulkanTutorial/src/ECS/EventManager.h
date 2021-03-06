#pragma once

#include <ECS\System.h>
#include <Types\RingBuffer.h>

#include <vector>
#include <array>
#include <map>
#include <set>
#include <unordered_map>
#include <string_view>
#include <cassert>
#include <typeindex>
#include <memory>

namespace Puffin
{
	namespace ECS
	{
		typedef uint8_t EventType;

		typedef std::set<std::shared_ptr<System>> SystemSet;

		class IEventQueue
		{
		public: 
			virtual ~IEventQueue() = default;
		};

		template<typename EventT>
		class EventQueue : public IEventQueue
		{
		public:

			// Store event in event buffer
			void Push(EventT event)
			{
				for (auto buffer : buffers)
				{
					buffer->Push(event);
				}
			}

			void Subscribe(std::shared_ptr<RingBuffer<EventT>> buffer)
			{
				buffers.push_back(buffer);
			}

		private:

			std::vector<std::shared_ptr<Puffin::RingBuffer<EventT>>> buffers;
		};

		class EventManager
		{
		public:

			template<typename EventT>
			void RegisterEvent()
			{
				std::type_index typeIndex = typeid(EventT);
				assert(eventTypes.find(typeIndex) == eventTypes.end() && "Registering event type more than once");

				// Add new event type to event type map
				eventTypes.insert({ typeIndex, nextEventType });

				// Create EventT type Queue pointer
				std::shared_ptr<EventQueue<EventT>> queue = std::make_shared<EventQueue<EventT>>();

				// Cast EventQueue pointer to IEventQueue and add to queue map
				eventQueues.insert({ typeIndex, std::static_pointer_cast<IEventQueue>(queue) });

				// Increment next event type
				nextEventType++;
			}

			template<typename EventT>
			void Publish(EventT event)
			{
				std::type_index typeIndex = typeid(EventT);
				assert(eventTypes.find(typeIndex) != eventTypes.end() && "Event Type not registered before use");

				GetEventQueue<EventT>()->Push(event);
			}

			template<typename EventT>
			void Subscribe(std::shared_ptr<RingBuffer<EventT>> buffer)
			{
				std::type_index typeIndex = typeid(EventT);
				assert(eventTypes.find(typeIndex) != eventTypes.end() && "Event Type not registered before use");

				GetEventQueue<EventT>()->Subscribe(buffer);
			}

		private:

			// Map from event type to event type integer
			std::unordered_map<std::type_index, EventType> eventTypes;

			// Map from event type to event queue
			std::unordered_map<std::type_index, std::shared_ptr<IEventQueue>> eventQueues;

			EventType nextEventType;

			template<typename EventT>
			std::shared_ptr<EventQueue<EventT>> GetEventQueue()
			{
				std::type_index typeIndex = typeid(EventT);
				assert(eventTypes.find(typeIndex) != eventTypes.end() && "Event Type not registered before use");

				return std::static_pointer_cast<EventQueue<EventT>>(eventQueues[typeIndex]);
			}
		};
	}
}