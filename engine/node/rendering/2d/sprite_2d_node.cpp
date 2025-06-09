#include "node/rendering/2d/sprite_2d_node.h"

#include "component/rendering/2d/sprite_component_2d.h"

namespace puffin::rendering
{
	void Sprite2DNode::Initialize()
	{
		Transform2DNode::Initialize();

		auto sprite = AddComponent<SpriteComponent2D>();
	}

	void Sprite2DNode::Deinitialize()
	{
		Transform2DNode::Deinitialize();

		RemoveComponent<SpriteComponent2D>();
	}

	const std::string& Sprite2DNode::GetTypeString() const
	{
		return gSpriteNode2DTypeString;
	}

	entt::id_type Sprite2DNode::GetTypeID() const
	{
		return reflection::GetTypeHashedString<Sprite2DNode>();
	}

	const Vector3f& Sprite2DNode::GetColour() const
	{
		return GetComponent<SpriteComponent2D>().colour;
	}

	Vector3f& Sprite2DNode::Colour()
	{
		return GetComponent<SpriteComponent2D>().colour;
	}

	void Sprite2DNode::SetColour(const Vector3f colour)
	{
		mRegistry->patch<SpriteComponent2D>(mEntity, [&colour](auto& sprite) { sprite.colour = colour; });
	}

	const Vector2f& Sprite2DNode::GetOffset() const
	{
		return GetComponent<SpriteComponent2D>().offset;
	}

	Vector2f& Sprite2DNode::Offset()
	{
		return GetComponent<SpriteComponent2D>().offset;
	}

	void Sprite2DNode::SetOffset(const Vector2f offset)
	{
		mRegistry->patch<SpriteComponent2D>(mEntity, [&offset](auto& sprite) { sprite.offset = offset; });
	}
}
