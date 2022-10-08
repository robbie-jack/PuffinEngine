#pragma once

#include <Rendering/Helper Classes/VKVertex.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace Puffin::Rendering
{
	enum class VertexFormat : uint8_t
	{
		Unknown = 0,
		PC_32,
		PNCTV_32,
		PNTV_32,
		P_64_NTV_32
	};

	struct Vertex_PC_32
	{
		glm::vec3 pos;
		glm::vec3 color;

		bool operator==(const Vertex_PC_32& other) const
		{
			return pos == other.pos && color == other.color;
		}

		static void GetVertexBindingAndAttributes(VkVertexInputBindingDescription& bindingDescription,
			std::vector<VkVertexInputAttributeDescription>& attributeDescriptions)
		{
			VKUtil::VertexBuilder::Begin()
				.BindSize(sizeof(Vertex_PC_32))
				.BindAttribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PC_32, pos))
				.BindAttribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PC_32, color))
				.Build(bindingDescription, attributeDescriptions);
		}
	};

	struct Vertex_PNTV_32
	{
		glm::vec3 pos = { 0.0f, 0.0f, 0.0f };
		glm::vec3 normal = { 0.0f, 0.0f, 0.0f };
		glm::vec3 tangent = { 0.0f, 0.0f, 0.0f };
		glm::vec2 uv = { 0.0f, 0.0f};

		bool operator==(const Vertex_PNTV_32& other) const
		{
			return pos == other.pos
				&& normal == other.normal
				&& tangent == other.tangent
				&& uv == other.uv;
		}

		static void GetVertexBindingAndAttributes(VkVertexInputBindingDescription& bindingDescription,
			std::vector<VkVertexInputAttributeDescription>& attributeDescriptions)
		{
			VKUtil::VertexBuilder::Begin()
				.BindSize(sizeof(Vertex_PNTV_32))
				.BindAttribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PNTV_32, pos))
				.BindAttribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PNTV_32, normal))
				.BindAttribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PNTV_32, tangent))
				.BindAttribute(VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex_PNTV_32, uv))
				.Build(bindingDescription, attributeDescriptions);
		}
	};

	struct Vertex_PNCTV_32
	{
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 uv;

		bool operator==(const Vertex_PNCTV_32& other) const
		{
			return pos == other.pos
				&& color == other.color
				&& normal == other.normal
				&& tangent == other.tangent
				&& uv == other.uv;
		}

		static void GetVertexBindingAndAttributes(VkVertexInputBindingDescription& bindingDescription,
			std::vector<VkVertexInputAttributeDescription>& attributeDescriptions)
		{
			VKUtil::VertexBuilder::Begin()
				.BindSize(sizeof(Vertex_PNCTV_32))
				.BindAttribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PNCTV_32, pos))
				.BindAttribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PNCTV_32, color))
				.BindAttribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PNCTV_32, normal))
				.BindAttribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PNCTV_32, tangent))
				.BindAttribute(VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex_PNCTV_32, uv))
				.Build(bindingDescription, attributeDescriptions);
		}
	};

	static VertexFormat ParseVertexFormatFromString(const char* f)
	{
		if (strcmp(f, "PNCTV_F32"))
		{
			return VertexFormat::PNCTV_32;
		}
		else if (strcmp(f, "PNTV_32"))
		{
			return VertexFormat::PNTV_32;
		}
		else if (strcmp(f, "P_64_NTV_32"))
		{
			return VertexFormat::P_64_NTV_32;
		}
		else
		{
			return VertexFormat::Unknown;
		}
	}

	static const char* ParseVertexStringFromFormat(VertexFormat format)
	{
		if (format == VertexFormat::PNCTV_32)
		{
			return "PNCTV_F32";
		}
		else if (format == VertexFormat::PNTV_32)
		{
			return "PNTV_32";
		}
		else if (format == VertexFormat::P_64_NTV_32)
		{
			return "P_64_NTV_32";
		}

		return "Invalid Format";
	}
}

namespace std
{
	template<> struct hash<Puffin::Rendering::Vertex_PNCTV_32>
	{
		size_t operator()(Puffin::Rendering::Vertex_PNCTV_32 const& vertex) const
		{
			return (hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1) ^
				(hash<glm::vec3>()(vertex.normal) << 1) ^
				(hash<glm::vec3>()(vertex.tangent) << 1) ^
				(hash<glm::vec2>()(vertex.uv) << 1) >> 1);
		}
	};

	template<> struct hash<Puffin::Rendering::Vertex_PNTV_32>
	{
		size_t operator()(Puffin::Rendering::Vertex_PNTV_32 const& vertex) const
		{
			return (hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1) ^
				(hash<glm::vec3>()(vertex.tangent) << 1) ^
				(hash<glm::vec2>()(vertex.uv) << 1) >> 1);
		}
	};
};