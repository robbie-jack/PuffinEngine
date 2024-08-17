#include "puffin/nodes/transformnode3d.h"

#include "puffin/components/transformcomponent3d.h"
#include "puffin/ecs/enttsubsystem.h"

namespace puffin
{
	TransformNode3D::TransformNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id) : Node(engine, id)
	{
		mName = "Transform3D";

		AddComponent<TransformComponent3D>();
	}

	void TransformNode3D::BeginPlay()
	{
		Node::BeginPlay();
	}

	void TransformNode3D::Update(const double delta_time)
	{
		Node::Update(delta_time);
	}

	void TransformNode3D::FixedUpdate(const double delta_time)
	{
		Node::FixedUpdate(delta_time);
	}

	void TransformNode3D::EndPlay()
	{
		Node::EndPlay();
	}

	bool TransformNode3D::HasTransform3D() const
	{
		return true;
	}

	const TransformComponent3D* TransformNode3D::GetTransform3D() const
	{
		return &GetComponent<TransformComponent3D>();
	}

	TransformComponent3D* TransformNode3D::GetTransform3D()
	{
		mTransformChanged = true;

		return &GetComponent<TransformComponent3D>();
	}

#ifdef PFN_DOUBLE_PRECISION
	const Vector3d& TransformNode3D::position() const
	{
		return transform_3d()->position;
	}

	Vector3d& TransformNode3D::position()
	{
		return transform_3d()->position;
	}

	void TransformNode3D::set_position(const Vector3d& position)
	{
		m_registry->patch<TransformComponent3D>(m_entity, [&position](auto& transform) { transform.position = position; });

		m_transform_changed = true;
	}
#else
	const Vector3f& TransformNode3D::position() const
	{
		return GetTransform3D()->position;
	}

	Vector3f& TransformNode3D::position()
	{
		return GetTransform3D()->position;
	}

	void TransformNode3D::set_position(const Vector3f& position)
	{
		mRegistry->patch<TransformComponent3D>(mEntity, [&position](auto& transform) { transform.position = position; });

		mTransformChanged = true;
	}
#endif

	const maths::Quat& TransformNode3D::orientation() const
	{
		return GetTransform3D()->orientationQuat;
	}

	maths::Quat& TransformNode3D::orientation()
	{
		return GetTransform3D()->orientationQuat;
	}

	void TransformNode3D::set_orientation(const maths::Quat& orientation)
	{
		mRegistry->patch<TransformComponent3D>(mEntity, [&orientation](auto& transform) { transform.orientationQuat = orientation; });

		mTransformChanged = true;
	}

	const maths::EulerAngles& TransformNode3D::euler_angles() const
	{
		return GetTransform3D()->orientationEulerAngles;
	}

	maths::EulerAngles& TransformNode3D::euler_angles()
	{
		return GetTransform3D()->orientationEulerAngles;
	}

	void TransformNode3D::set_euler_angles(const maths::EulerAngles& eulerAngles)
	{
		mRegistry->patch<TransformComponent3D>(mEntity, [&eulerAngles](auto& transform) { transform.orientationEulerAngles = eulerAngles; });

		mTransformChanged = true;
	}

	const Vector3f& TransformNode3D::scale() const
	{
		return GetTransform3D()->scale;
	}

	Vector3f& TransformNode3D::scale()
	{
		return GetTransform3D()->scale;
	}

	void TransformNode3D::set_scale(const Vector3f& scale)
	{
		mRegistry->patch<TransformComponent3D>(mEntity, [&scale](auto& transform) { transform.scale = scale; });

		mTransformChanged = true;
	}
}
