#include "EntityView.h"

namespace Puffin::ECS
{
	template <typename... ComponentTypes>
	void EntityView<ComponentTypes...>::Init()
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
}