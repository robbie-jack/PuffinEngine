#pragma once

#ifndef RIGIDBODY_COMPONENT_2D_H
#define RIGIDBODY_COMPONENT_2D_H

//#include <Types/Quat.h>

namespace Puffin
{
	namespace Physics
	{
		struct RigidbodyComponent2D
		{
			RigidbodyComponent2D() {}

			RigidbodyComponent2D(Float InInvMass) :
				invMass(InInvMass)
			{
			}
			
			Vector2 linearVelocity = Vector2(0.0f);
			Vector2 angularVelocity = Vector2(0.0f);

			Float invMass = 0.0f;
			Float elasticity = 1.0f;

			//Quat orientation;
		};

		static inline void ApplyLinearImpulse(RigidbodyComponent2D& Body, const Vector2& Impulse)
		{
			// Apply Accumulated Impulses for this frame if body does not have infinite mass
			if (Body.invMass == 0.0f)
				return;

			// Apply Accumulated Impulse
			Body.linearVelocity += Impulse * Body.invMass;
		}

		static inline void ApplyAngularImpulse(RigidbodyComponent2D& Body, const Vector2& Impulse)
		{
			
		}

		template<class Archive>
		void serialize(Archive& archive, RigidbodyComponent2D& body)
		{
			archive(body.invMass);
			archive(body.elasticity);
		}
	}
}

#endif //RIGIDBODY_COMPONENT_2D_H