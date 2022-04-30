#pragma once

namespace Puffin
{
	namespace Physics
	{
		struct RigidbodyComponent2D
		{
			RigidbodyComponent2D() {}

			RigidbodyComponent2D(float InInvMass) :
				invMass(InInvMass)
			{
			}

			Vector2f linearVelocity = Vector2f(0.0f);
			float angularVelocity = 0.0f;

			float invMass = 0.0f;
			float elasticity = 1.0f;
		};
	}
}