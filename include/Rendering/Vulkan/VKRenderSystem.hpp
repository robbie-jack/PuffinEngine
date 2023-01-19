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

namespace Puffin::Rendering::VK
{
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
		void Update() override {}
		void Stop() override {}
		void Cleanup() override {}

	private:

		// Initialization Members
		vku::Framework m_framework;
		vk::Device m_device;
		vku::Window m_window;

		// Indicated initialization completed without any failures
		bool m_isInitialized = false;

		void InitVulkan();

	};
}