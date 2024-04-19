#pragma once

#include "scene/node.h"
#include "Types/Quat.h"
#include "Types/Vector.h"

namespace puffin
{
	struct TransformComponent3D;
}

namespace puffin::scene
{
	class TransformNode3D : public Node
	{
	public:

		explicit TransformNode3D(const std::shared_ptr<core::Engine>& engine, const PuffinID& id = gInvalidID);
		~TransformNode3D() override = default;

		void begin_play() override;
		void update(const double delta_time) override;
		void physics_update(const double delta_time) override;
		void end_play() override;

		TransformComponent3D& get_transform();
		Vector3f& position();
		maths::Quat& orientation();
		Vector3f& scale();

	protected:



	};
}
