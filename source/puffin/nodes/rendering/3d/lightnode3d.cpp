#include "puffin/nodes/rendering/3d/lightnode3d.h"

#include "puffin/components/rendering/3d/shadowcastercomponent3d.h"

namespace puffin::rendering
{
	LightNode3D::LightNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id) : TransformNode3D(engine, id)
	{
	}

	bool LightNode3D::GetCastShadows() const
	{
		return mCastShadows;
	}

	void LightNode3D::SetCastShadows(bool castShadows)
	{
		if (mCastShadows == castShadows)
			return;

		if (castShadows == true)
		{
			if (!HasComponent<ShadowCasterComponent3D>())
			{
				AddComponent<ShadowCasterComponent3D>();
			}
		}
		else
		{
			if (HasComponent<ShadowCasterComponent3D>())
			{
				RemoveComponent<ShadowCasterComponent3D>();
			}
		}

		mCastShadows = castShadows;
	}
}
