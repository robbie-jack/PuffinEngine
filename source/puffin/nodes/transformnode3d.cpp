#include "puffin/nodes/transformnode3d.h"

#include "puffin/components/transformcomponent3d.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/scene/scenegraphsubsystem.h"

namespace puffin
{
	TransformNode3D::TransformNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id) : Node(engine, id)
	{
		mName = "Transform3D";

		AddComponent<TransformComponent3D>();
	}

	TransformNode3D::~TransformNode3D()
	{
		RemoveComponent<TransformComponent3D>();
	}

	const TransformComponent3D& TransformNode3D::GetTransform() const
	{
		return GetComponent<TransformComponent3D>();
	}

	TransformComponent3D& TransformNode3D::Transform()
	{
		mEngine->GetSubsystem<scene::SceneGraphSubsystem>()->NotifyTransformChanged(mNodeID);

		return GetComponent<TransformComponent3D>();
	}

	const TransformComponent3D& TransformNode3D::GetGlobalTransform() const
	{
		return mEngine->GetSubsystem<scene::SceneGraphSubsystem>()->GetNodeGlobalTransform3D(mNodeID);
	}

	/*TransformComponent3D& TransformNode3D::GlobalTransform()
	{
		
	}*/


#ifdef PFN_DOUBLE_PRECISION
	const Vector3d& TransformNode3D::position() const
	{
		return GetTransform().position;
	}

	Vector3d& TransformNode3D::position()
	{
		return Transform().position;
	}

	void TransformNode3D::set_position(const Vector3d& position)
	{
		mRegistry->patch<TransformComponent3D>(mEntity, [&position](auto& transform) { transform.position = position; });

		mEngine->GetSubsystem<scene::SceneGraphSubsystem>()->NotifyTransformChanged(mNodeID);
	}
#else
	const Vector3f& TransformNode3D::GetPosition() const
	{
		return GetTransform().position;
	}

	Vector3f& TransformNode3D::Position()
	{
		return Transform().position;
	}

	void TransformNode3D::SetPosition(const Vector3f& position)
	{
		mRegistry->patch<TransformComponent3D>(mEntity, [&position](auto& transform) { transform.position = position; });

		mEngine->GetSubsystem<scene::SceneGraphSubsystem>()->NotifyTransformChanged(mNodeID);
	}
#endif

	const maths::Quat& TransformNode3D::GetOrientationQuat() const
	{
		return GetTransform().orientationQuat;
	}

	maths::Quat& TransformNode3D::OrientationQuat()
	{
		return Transform().orientationQuat;
	}

	void TransformNode3D::SetOrientationQuat(const maths::Quat& orientation)
	{
		mRegistry->patch<TransformComponent3D>(mEntity, [&orientation](auto& transform) { transform.orientationQuat = orientation; });

		mEngine->GetSubsystem<scene::SceneGraphSubsystem>()->NotifyTransformChanged(mNodeID);
	}

	const maths::EulerAngles& TransformNode3D::GetOrientationEulerAngles() const
	{
		return GetTransform().orientationEulerAngles;
	}

	maths::EulerAngles& TransformNode3D::OrientationEulerAngles()
	{
		return Transform().orientationEulerAngles;
	}

	void TransformNode3D::SetOrientationEulerAngles(const maths::EulerAngles& eulerAngles)
	{
		mRegistry->patch<TransformComponent3D>(mEntity, [&eulerAngles](auto& transform) { transform.orientationEulerAngles = eulerAngles; });

		mEngine->GetSubsystem<scene::SceneGraphSubsystem>()->NotifyTransformChanged(mNodeID);
	}

	const Vector3f& TransformNode3D::SetScale() const
	{
		return GetTransform().scale;
	}

	Vector3f& TransformNode3D::Scale()
	{
		return Transform().scale;
	}

	void TransformNode3D::SetScale(const Vector3f& scale)
	{
		mRegistry->patch<TransformComponent3D>(mEntity, [&scale](auto& transform) { transform.scale = scale; });

		mEngine->GetSubsystem<scene::SceneGraphSubsystem>()->NotifyTransformChanged(mNodeID);
	}
}
