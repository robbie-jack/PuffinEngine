#pragma once

#include "puffin/types/Vector.h"
#include "nlohmann/json.hpp"

#include <vector>

namespace puffin::rendering
{
	namespace util
	{
		class VertexLayout;
	}

	enum class VertexFormat : uint8_t
	{
		Unknown = 0,
		PC32,
		PNC32,
		PNTV32,
		P64NTV32
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(VertexFormat,
	{
		{ VertexFormat::Unknown, "Unknown" },
		{ VertexFormat::PC32, "PC32" },
		{ VertexFormat::PNC32, "PNC32" },
		{ VertexFormat::PNTV32, "PNTV32" },
		{ VertexFormat::P64NTV32, "P64NTV32" }
	});

	struct VertexPC32
	{
		Vector3f pos;
		Vector3f color;

		bool operator==(const VertexPC32& other) const
		{
			return pos == other.pos && color == other.color;
		}

		static util::VertexLayout getLayoutVK();
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

		static util::VertexLayout getLayoutVK();
	};

	struct VertexPNTV32
	{
		Vector3f pos = { 0.0f, 0.0f, 0.0f };
		float uvX;
		Vector3f normal = { 0.0f, 0.0f, 0.0f };
		float uvY;
		Vector3f tangent = { 0.0f, 0.0f, 0.0f };
		float pad0;

		bool operator==(const VertexPNTV32& other) const
		{
			return pos == other.pos
				&& normal == other.normal
				&& tangent == other.tangent
				&& uvX == other.uvX
				&& uvY == other.uvY;
		}

		static util::VertexLayout getLayoutVK();
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

		static util::VertexLayout getLayoutVK();
	};

	static VertexFormat parseVertexFormatFromString(const char* f)
	{
		if (strcmp(f, "PNTV32") == 0)
		{
			return VertexFormat::PNTV32;
		}

		if (strcmp(f, "P64NTV32") == 0)
		{
			return VertexFormat::P64NTV32;
		}

		return VertexFormat::Unknown;
	}

	static const char* parseVertexStringFromFormat(const VertexFormat format)
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

	static uint32_t parseVertexSizeFromFormat(const VertexFormat format)
	{
		if (format == VertexFormat::PC32)
		{
			return sizeof(VertexPC32);
		}

		if (format == VertexFormat::PNC32)
		{
			return sizeof(VertexPNC32);
		}

		if (format == VertexFormat::PNTV32)
		{
			return sizeof(VertexPNTV32);
		}

		if (format == VertexFormat::P64NTV32)
		{
			return sizeof(VertexP64NTV32);
		}

		return 0;
	}
}

namespace std
{
	template<> struct hash<puffin::rendering::VertexPNTV32>
	{
		size_t operator()(puffin::rendering::VertexPNTV32 const& vertex) const noexcept
		{
			return (hash<puffin::Vector3f>()(vertex.pos) ^
				(hash<puffin::Vector3f>()(vertex.normal) << 1) ^
				(hash<puffin::Vector3f>()(vertex.tangent) << 1) ^
				(hash<puffin::Vector2f>()(vertex.uvX) << 1) ^
				(hash<puffin::Vector2f>()(vertex.uvY) << 1) >> 1);
		}
	};

	template<> struct hash<puffin::rendering::VertexP64NTV32>
	{
		size_t operator()(puffin::rendering::VertexP64NTV32 const& vertex) const noexcept
		{
			return (hash<puffin::Vector3d>()(vertex.pos) ^
				(hash<puffin::Vector3f>()(vertex.normal) << 1) ^
				(hash<puffin::Vector3f>()(vertex.tangent) << 1) ^
				(hash<puffin::Vector2f>()(vertex.uv) << 1) >> 1);
		}
	};
};