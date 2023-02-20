#pragma once

#include "Types/Vector.h"

#include <vector>

namespace vk
{
	struct VertexInputBindingDescription;
	struct VertexInputAttributeDescription;
}

namespace bgfx
{
	struct VertexLayout;
}

namespace Puffin::Rendering
{
	enum class VertexFormat : uint8_t
	{
		Unknown = 0,
		PC32,
		PNC32,
		PNTV32,
		P64NTV32
	};

	struct VertexPC32
	{
		Vector3f pos;
		Vector3f color;

		bool operator==(const VertexPC32& other) const
		{
			return pos == other.pos && color == other.color;
		}

		static bgfx::VertexLayout GetLayout();

		static void GetVertexBindingAndAttributes(vk::VertexInputBindingDescription& bindingDescription,
			std::vector<vk::VertexInputAttributeDescription>& attributeDescriptions);
	};

	struct VertexPNC32
	{
		Vector3f pos;
		Vector3f normal;
		Vector3f color;

		bool operator==(const VertexPNC32& other) const
		{
			return pos == other.pos && normal == other.normal && color == other.color;
		}

		static bgfx::VertexLayout GetLayout();

		static void GetVertexBindingAndAttributes(vk::VertexInputBindingDescription& bindingDescription,
			std::vector<vk::VertexInputAttributeDescription>& attributeDescriptions);
	};

	struct VertexPNTV32
	{
		Vector3f pos = { 0.0f, 0.0f, 0.0f };
		Vector3f normal = { 0.0f, 0.0f, 0.0f };
		Vector3f tangent = { 0.0f, 0.0f, 0.0f };
		Vector2f uv = { 0.0f, 0.0f};

		bool operator==(const VertexPNTV32& other) const
		{
			return pos == other.pos
				&& normal == other.normal
				&& tangent == other.tangent
				&& uv == other.uv;
		}

		static bgfx::VertexLayout GetLayout();

		static void GetVertexBindingAndAttributes(vk::VertexInputBindingDescription& bindingDescription,
			std::vector<vk::VertexInputAttributeDescription>& attributeDescriptions);
	};

	struct VertexP64NTV32
	{
		Vector3d pos = { 0.0, 0.0, 0.0 };
		Vector3f normal = { 0.0f, 0.0f, 0.0f };
		Vector3f tangent = { 0.0f, 0.0f, 0.0f };
		Vector2f uv = { 0.0f, 0.0f };

		bool operator==(const VertexP64NTV32& other) const
		{
			return pos == other.pos
				&& normal == other.normal
				&& tangent == other.tangent
				&& uv == other.uv;
		}

		static bgfx::VertexLayout GetLayout();

		static void GetVertexBindingAndAttributes(vk::VertexInputBindingDescription& bindingDescription,
			std::vector<vk::VertexInputAttributeDescription>& attributeDescriptions);
	};

	static VertexFormat ParseVertexFormatFromString(const char* f)
	{
		if (strcmp(f, "PNTV32") == 0)
		{
			return VertexFormat::PNTV32;
		}
		else if (strcmp(f, "P64NTV32") == 0)
		{
			return VertexFormat::P64NTV32;
		}
		else
		{
			return VertexFormat::Unknown;
		}
	}

	static const char* ParseVertexStringFromFormat(VertexFormat format)
	{
		if (format == VertexFormat::PNTV32)
		{
			return "PNTV32";
		}
		else if (format == VertexFormat::P64NTV32)
		{
			return "P64NTV32";
		}

		return "Invalid Format";
	}

	static uint32_t GetVertexSizeFromFormat(VertexFormat format)
	{
		if (format == VertexFormat::PNTV32)
		{
			return sizeof(VertexPNTV32);
		}
		else if (format == VertexFormat::P64NTV32)
		{
			return sizeof(VertexP64NTV32);
		}

		return 0;
	}
}

namespace std
{
	template<> struct hash<Puffin::Rendering::VertexPNTV32>
	{
		size_t operator()(Puffin::Rendering::VertexPNTV32 const& vertex) const
		{
			return (hash<Puffin::Vector3f>()(vertex.pos) ^
				(hash<Puffin::Vector3f>()(vertex.normal) << 1) ^
				(hash<Puffin::Vector3f>()(vertex.tangent) << 1) ^
				(hash<Puffin::Vector2f>()(vertex.uv) << 1) >> 1);
		}
	};

	template<> struct hash<Puffin::Rendering::VertexP64NTV32>
	{
		size_t operator()(Puffin::Rendering::VertexP64NTV32 const& vertex) const
		{
			return (hash<Puffin::Vector3d>()(vertex.pos) ^
				(hash<Puffin::Vector3f>()(vertex.normal) << 1) ^
				(hash<Puffin::Vector3f>()(vertex.tangent) << 1) ^
				(hash<Puffin::Vector2f>()(vertex.uv) << 1) >> 1);
		}
	};
};