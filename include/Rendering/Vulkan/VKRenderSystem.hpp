#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

//#define VMA_IMPLEMENTATION

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

#include <vulkan/vulkan.hpp>
//#include <vma/vk_mem_alloc.h>

//#include "vk_mem_alloc.hpp"
#include "vku/vku_framework.hpp"
#include "vku/vku.hpp"

#include "ECS/System.hpp"
#include "Types/Vertex.hpp"

namespace Puffin::Rendering::VK
{
	static std::vector<VertexPC32> s_triangleVertices =
	{
		{ {0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
		{{ 0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},

		{{ 0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		{{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}},
	};

	//static VertexPC32 s_cubeVertices[] =
	//{
	//	{{-1.0f,  1.0f,  1.0f}, 0xff000000 },
	//	{ {1.0f,  1.0f,  1.0f}, 0xff0000ff },
	//	{{-1.0f, -1.0f,  1.0f}, 0xff00ff00 },
	//	{ {1.0f, -1.0f,  1.0f}, 0xff00ffff },
	//	{{-1.0f,  1.0f, -1.0f}, 0xffff0000 },
	//	{ {1.0f,  1.0f, -1.0f}, 0xffff00ff },
	//	{{-1.0f, -1.0f, -1.0f}, 0xffffff00 },
	//	{ {1.0f, -1.0f, -1.0f}, 0xffffffff },
	//};

	//static const uint16_t s_cubeTriList[] =
	//{
	//	0, 1, 2, // 0
	//	1, 3, 2,
	//	4, 6, 5, // 2
	//	5, 6, 7,
	//	0, 2, 4, // 4
	//	4, 2, 6,
	//	1, 5, 3, // 6
	//	5, 7, 3,
	//	0, 4, 1, // 8
	//	4, 5, 1,
	//	2, 3, 6, // 10
	//	6, 3, 7,
	//};

	// Vulkan Rendering System
	class VKRenderSystem : public ECS::System
	{
	public:

		VKRenderSystem()
		{
			m_systemInfo.name = "VKRenderSystem";
			m_systemInfo.updateOrder = Core::UpdateOrder::Render;
		}

		void Init() override;
		void PreStart() override {}
		void Start() override {}
		void Update() override;
		void Stop() override {}
		void Cleanup() override;

	private:

		// Initialization Members
		vku::Framework m_framework;
		vk::Device m_device;
		vku::Window m_window;

		vku::HostVertexBuffer m_triVertexBuffer;

		vku::ShaderModule m_vertMod, m_fragMod;
		vk::UniquePipeline m_triPipeline;

		// Indicated initialization completed without any failures
		bool m_isInitialized = false;

		void InitVulkan();
		vk::UniquePipeline BuildTrianglePipeline();
		void InitTriangleRender();

	};
}
