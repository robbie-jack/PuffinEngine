#include "node/rendering/2d/sprite_2d_node.h"

#include "component/rendering/2d/sprite_component_2d.h"

namespace puffin::rendering
{
	void SpriteNode2D::Initialize()
	{
		TransformNode2D::Initialize();

		auto sprite = AddComponent<SpriteComponent2D>();
	}

	void SpriteNode2D::Deinitialize()
	{
		TransformNode2D::Deinitialize();

		RemoveComponent<SpriteComponent2D>();
	}

	const std::string& SpriteNode2D::GetTypeString() const
	{
		return gSpriteNode2DTypeString;
	}

	entt::id_type SpriteNode2D::GetTypeID() const
	{
		return reflection::GetTypeHashedString<SpriteNode2D>();
	}

	const Vector3f& SpriteNode2D::GetColour() const
	{
		return GetComponent<SpriteComponent2D>().colour;
	}

	Vector3f& SpriteNode2D::Colour()
	{
		return GetComponent<SpriteComponent2D>().colour;
	}

	void SpriteNode2D::SetColour(const Vector3f colour)
	{
		mRegistry->patch<SpriteComponent2D>(mEntity, [&colour](auto& sprite) { sprite.colour = colour; });
	}

	const Vector2f& SpriteNode2D::GetOffset() const
	{
		return GetComponent<SpriteComponent2D>().offset;
	}

	Vector2f& SpriteNode2D::Offset()
	{
		return GetComponent<SpriteComponent2D>().offset;
	}

	void SpriteNode2D::SetOffset(const Vector2f offset)
	{
		mRegistry->patch<SpriteComponent2D>(mEntity, [&offset](auto& sprite) { sprite.offset = offset; });
	}
}
