#pragma once

#include "VKTypes.h"

#include <memory>
#include <fstream>

#include "Types/DeletionQueue.hpp"

namespace Puffin::Rendering
{
	// Forward Declaration
	class VulkanRenderSystem;

	class VKBaseRenderer
	{
	public:

		VKBaseRenderer() = default;
		virtual ~VKBaseRenderer() { m_vulkanRenderSystem = nullptr; }

		virtual void Setup(std::shared_ptr<VulkanRenderSystem> vulkanRenderSystem) = 0;
		virtual VkSemaphore& DrawScene(const int& frameIndex, const int& swapchainIndex) = 0;
		virtual void Cleanup() = 0;

	protected:

		std::shared_ptr<VulkanRenderSystem> m_vulkanRenderSystem = nullptr;

		DeletionQueue m_mainDeletionQueue;

		static inline std::vector<char> ReadFile(const std::string& filename)
		{
			std::ifstream file(filename, std::ios::ate | std::ios::binary);

			if (!file.is_open())
			{
				throw std::runtime_error("failed to open file!");
			}

			size_t fileSize = (rsize_t)file.tellg();
			std::vector<char> buffer(fileSize);

			file.seekg(0);
			file.read(buffer.data(), fileSize);

			file.close();

			//std::cout << "BufferSize: " << buffer.size() << std::endl;

			return buffer;
		}
	};
}
