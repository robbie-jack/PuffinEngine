#include "puffin/nodes/transformnode2d.h"

#include "puffin/core/engine.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/components/transformcomponent2d.h"
#include "puffin/scene/scenegraphsubsystem.h"

namespace puffin::core
{
	class Engine;
}

namespace puffin
{
	void TransformNode2D::Initialize()
	{
		Node::Initialize();

		AddComponent<TransformComponent2D>();
	}

	void TransformNode2D::Deinitialize()
	{
		Node::Deinitialize();

		RemoveComponent<TransformComponent2D>();
	}

	const std::string& TransformNode2D::GetTypeString() const
	{
		return gTransformNode2DTypeString;
	}

	entt::id_type TransformNode2D::GetTypeID() const
	{
		return reflection::GetTypeHashedString<TransformNode2D>();
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

	/*TransformComponent2D& TransformNode2D::GlobalTransform()
	{

	}*/

#ifdef PFN_DOUBLE_PRECISION
	const Vector2d& TransformNode2D::GetPosition() const
	{
		return GetTransform().position;
	}

	Vector2d& TransformNode2D::Position()
	{
		return Transform().position;
	}

	void TransformNode2D::SetPosition(const Vector2d& position)
	{
		mRegistry->patch<TransformComponent2D>(mEntity, [&position](auto& transform) { transform.position = position; });

		mEngine->GetSubsystem<scene::SceneGraphSubsystem>()->NotifyTransformChanged(mNodeID);
	}
#else
	

	const Vector2f& TransformNode2D::GetPosition() const
	{
		return GetTransform().position;
	}

	Vector2f& TransformNode2D::Position()
	{
		return Transform().position;
	}

	void TransformNode2D::SetPosition(const Vector2f& position)
	{
		mRegistry->patch<TransformComponent2D>(mEntity, [&position](auto& transform) { transform.position = position; });

		mEngine->GetSubsystem<scene::SceneGraphSubsystem>()->NotifyTransformChanged(mNodeID);
	}
#endif

	const float& TransformNode2D::GetRotation() const
	{
		return GetTransform().rotation;
	}

	float& TransformNode2D::Rotation()
	{
		return Transform().rotation;
	}

	void TransformNode2D::SetRotation(const float& rotation)
	{
		mRegistry->patch<TransformComponent2D>(mEntity, [&rotation](auto& transform) { transform.rotation = rotation; });

		mEngine->GetSubsystem<scene::SceneGraphSubsystem>()->NotifyTransformChanged(mNodeID);
	}

	const Vector2f& TransformNode2D::GetScale() const
	{
		return GetTransform().scale;
	}

	Vector2f& TransformNode2D::Scale()
	{
		return Transform().scale;
	}

	void TransformNode2D::SetScale(const Vector2f& scale)
	{
		mRegistry->patch<TransformComponent2D>(mEntity, [&scale](auto& transform) { transform.scale = scale; });

		mEngine->GetSubsystem<scene::SceneGraphSubsystem>()->NotifyTransformChanged(mNodeID);
	}
}
