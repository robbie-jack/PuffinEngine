#include "node/rendering/2d/sprite_2d_node.h"

#include "component/rendering/2d/sprite_component_2d.h"

namespace puffin::rendering
{
	void Sprite2DNode::Initialize()
	{
		Transform2DNode::Initialize();
	}

	void Sprite2DNode::Deinitialize()
	{
		Transform2DNode::Deinitialize();
	}

	void Sprite2DNode::Serialize(nlohmann::json& json) const
	{
		Transform2DNode::Serialize(json);

		nlohmann::json spriteJson;
		spriteJson["colour"] = serialization::Serialize(mColour);
		spriteJson["offset"] = serialization::Serialize(mOffset);

		json["sprite"] = spriteJson;
	}

	void Sprite2DNode::Deserialize(const nlohmann::json& json)
	{
		Transform2DNode::Deserialize(json);

		auto& spriteJson = json["sprite"];

		mColour = serialization::Deserialize<Vector3f>(spriteJson["colour"]);
		mOffset = serialization::Deserialize<Vector2f>(spriteJson["offset"]);
	}

	std::string_view Sprite2DNode::GetTypeString() const
	{
		return reflection::GetTypeString<Sprite2DNode>();
	}

	entt::id_type Sprite2DNode::GetTypeID() const
	{
		return reflection::GetTypeHashedString<Sprite2DNode>();
	}

	const Vector3f& Sprite2DNode::GetColour() const
	{
		return mColour;
	}

	Vector3f& Sprite2DNode::Colour()
	{
		return mColour;
	}

	void Sprite2DNode::SetColour(const Vector3f colour)
	{
		mColour = colour;
	}

	const Vector2f& Sprite2DNode::GetOffset() const
	{
		return mOffset;
	}

	Vector2f& Sprite2DNode::Offset()
	{
		return mOffset;
	}

	void Sprite2DNode::SetOffset(const Vector2f offset)
	{
		mOffset = offset;
	}
}
