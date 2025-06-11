#pragma once

#include "node/physics/2d/shape_2d_node.h"

namespace puffin::physics
{
	class Box2DNode : public Shape2DNode
	{
	public:

		void Serialize(nlohmann::json& json) const override;
		void Deserialize(const nlohmann::json& json) override;

		const Vector2f& GetHalfExtent() const;
		Vector2f& HalfExtent();
		void SetHalfExtent(const Vector2f& halfExtent);

	private:

		Vector2f mHalfExtent = { 0.5f };

	};
}