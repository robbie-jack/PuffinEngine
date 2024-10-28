#pragma once

#include "puffin/nodes/transformnode3d.h"
#include "puffin/utility/reflection.h"
#include "puffin/components/physics/3d/rigidbodycomponent3d.h"

namespace puffin
{
	
	
	namespace physics
	{
		const std::string gRigidbodyNode3DTypeString = "RigidbodyNode3D";
		const entt::id_type gRigidbodyNode3DTypeID = entt::hs(gRigidbodyNode3DTypeString.c_str());
		
		class RigidbodyNode3D : public TransformNode3D
		{
		public:

			void Initialize() override;
			void Deinitialize() override;

			[[nodiscard]] const BodyType& GetBodyType() const;
			void SetBodyType(const BodyType& bodyType);

			[[nodiscard]] const float& GetMass() const;
			void SetMass(const float& mass);

			[[nodiscard]] const float& GetElasticity() const;
			void SetElasticity(const float& elasticity);

		};
	}

	template<>
	inline void reflection::RegisterType<physics::RigidbodyNode3D>()
	{
		using namespace physics;

		entt::meta<RigidbodyNode3D>()
			.type(gRigidbodyNode3DTypeID)
			.base<TransformNode3D>()
			.custom<NodeCustomData>(gRigidbodyNode3DTypeString);
	}
}