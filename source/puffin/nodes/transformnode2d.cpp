#include "puffin/nodes/transformnode2d.h"

#include "puffin/core/engine.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/components/transformcomponent2d.h"
#include "puffin/scene/scenegraph.h"

namespace puffin::core
{
	class Engine;
}

namespace puffin
{
	TransformNode2D::TransformNode2D(const std::shared_ptr<core::Engine>& engine, const UUID& id) : Node(engine, id)
	{
		mName = "TransformNode2D";

		AddComponent<TransformComponent2D>();
	}

	const TransformComponent2D& TransformNode2D::GetTransform() const
	{
		return GetComponent<TransformComponent2D>();
	}

	TransformComponent2D& TransformNode2D::Transform()
	{
		mEngine->GetSubsystem<scene::SceneGraphSubsystem>()->NotifyTransformChanged(mNodeID);

		return GetComponent<TransformComponent2D>();
	}

	const TransformComponent2D& TransformNode2D::GetGlobalTransform() const
	{
		return mEngine->GetSubsystem<scene::SceneGraphSubsystem>()->GetNodeGlobalTransform2D(mNodeID);
	}

	TransformComponent2D& TransformNode2D::GlobalTransform()
	{

	}

#ifdef PFN_DOUBLE_PRECISION
	const Vector2d& TransformNode2D::GetPosition() const
	{
		return Transform2D()->position;
	}

	Vector2d& TransformNode2D::Position()
	{
		return transform_2d()->position;
	}

	void TransformNode2D::SetPosition(const Vector2d& position)
	{
		m_registry->patch<TransformComponent2D>(m_entity, [&position](auto& transform) { transform.position = position; });

		m_transform_changed = true;
	}
#else
	

	const Vector2f& TransformNode2D::GetPosition() const
	{
		return GetTransform2D()->position;
	}

	Vector2f& TransformNode2D::Position()
	{
		return GetTransform2D()->position;
	}

	void TransformNode2D::SetPosition(const Vector2f& position)
	{
		mRegistry->patch<TransformComponent2D>(mEntity, [&position](auto& transform) { transform.position = position; });

		mTransformChanged = true;
	}
#endif

	const float& TransformNode2D::GetRotation() const
	{
		return GetTransform2D()->rotation;
	}

	float& TransformNode2D::Rotation()
	{
		return GetTransform2D()->rotation;
	}

	void TransformNode2D::SetRotation(const float& rotation)
	{
		mRegistry->patch<TransformComponent2D>(mEntity, [&rotation](auto& transform) { transform.rotation = rotation; });

		mTransformChanged = true;
	}

	const Vector2f& TransformNode2D::GetScale() const
	{
		return GetTransform2D()->scale;
	}

	Vector2f& TransformNode2D::Scale()
	{
		return GetTransform2D()->scale;
	}

	void TransformNode2D::SetScale(const Vector2f& scale)
	{
		mRegistry->patch<TransformComponent2D>(mEntity, [&scale](auto& transform) { transform.scale = scale; });

		mTransformChanged = true;
	}
}
