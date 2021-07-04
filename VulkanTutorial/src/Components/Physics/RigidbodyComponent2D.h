#pragma once

#ifndef RIGIDBODY_COMPONENT_2D_H
#define RIGIDBODY_COMPONENT_2D_H

#include <Components/BaseComponent.h>

namespace Puffin
{
	namespace Physics
	{
		struct RigidbodyComponent2D : public BaseComponent
		{
			RigidbodyComponent2D()
			{
				bFlagCreated = false;
				bFlagDeleted = false;
			}

			RigidbodyComponent2D(float InMass) :
				mass(InMass)
			{
				RigidbodyComponent2D();
			}
			
			Vector2 velocity = Vector2(0.0f);
			Vector2 acceleration = Vector2(0.0f); // Acceleration last physics tick
			Vector2 force = Vector2(0.0f);
			float mass = 0.0f;

		};

		template<class Archive>
		void serialize(Archive& archive, RigidbodyComponent2D& comp)
		{
			archive(comp.mass);
		}
	}
}

#endif //RIGIDBODY_COMPONENT_2D_H