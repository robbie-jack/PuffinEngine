#pragma once

#include <functional>
#include <memory>

#include "puffin/rendering/vulkan/typesvk.h"
#include "puffin/types/quat.h"
#include "puffin/types/vector3.h"

namespace puffin::rendering
{
	class RenderSubsystemVK;
}

namespace puffin::rendering::util
{
	const std::unordered_map<uint8_t, vk::SampleCountFlagBits> gSampleCountToFlagBits =
	{
		{ 1, vk::SampleCountFlagBits::e1 },
		{ 2, vk::SampleCountFlagBits::e2 },
		{ 4, vk::SampleCountFlagBits::e4 },
		{ 8, vk::SampleCountFlagBits::e8 },
		{ 16, vk::SampleCountFlagBits::e16 },
		{ 32, vk::SampleCountFlagBits::e32 },
		{ 64, vk::SampleCountFlagBits::e64 }
	};

	const std::unordered_map<vk::Result, std::string> gResultToString =
	{
		{ vk::Result::eSuccess, "Success" },
		{ vk::Result::eErrorDeviceLost, "Device Lost" },
		{ vk::Result::eErrorOutOfHostMemory, "Out of Host Memory"},
		{ vk::Result::eErrorOutOfDeviceMemory, "Out of Device Memory"},
		{ vk::Result::eTimeout, "Timeout"}
	};

	void CheckResult(vk::Result result);

	void ImmediateSubmit(const RenderSubsystemVK* renderSystem, std::function<void(VkCommandBuffer cmd)>&& function);

	struct CopyDataBetweenBuffersParams
	{
		size_t dataSize;
		vk::Buffer srcBuffer;
		vk::Buffer dstBuffer;
		size_t srcOffset = 0;
		size_t dstOffset = 0;
	};

	void CopyDataBetweenBuffers(const RenderSubsystemVK* renderSystem, const CopyDataBetweenBuffersParams& params);

	struct CopyCPUDataIntoGPUBufferParams
	{
		AllocatedBuffer dstBuffer;
		size_t dataSize;
		void* srcData;
		size_t srcOffset = 0;
		size_t dstOffset = 0;
	};

	void CopyCPUDataIntoGPUBuffer(const RenderSubsystemVK* renderSystem, const CopyCPUDataIntoGPUBufferParams& params);

	struct CreateBufferParams
	{
		size_t allocSize;
		vk::BufferUsageFlags bufferUsage;
		vma::MemoryUsage memoryUsage = vma::MemoryUsage::eAuto;
		vma::AllocationCreateFlags allocFlags = {};
		vk::MemoryPropertyFlags requiredFlags = {};
	};

	AllocatedBuffer CreateBuffer(const vma::Allocator& allocator, const CreateBufferParams& params);

	struct CreateImageParams
	{
		vk::ImageCreateInfo imageInfo;
		vk::ImageViewCreateInfo imageViewInfo;
	};

	AllocatedImage CreateImage(const RenderSubsystemVK* renderSystem, const CreateImageParams& params);

	struct CreateFormattedImageParams
	{
		vk::Extent3D extent;
		vk::Format format;
	};

	AllocatedImage CreateColorImage(const RenderSubsystemVK* renderSystem, const CreateFormattedImageParams& params);
	AllocatedImage CreateDepthImage(const RenderSubsystemVK* renderSystem, const CreateFormattedImageParams& params);

	struct InitTextureParams
	{
		void* pixelData;
		size_t dataSize;
		uint32_t width;
		uint32_t height;
		vk::Format format;
	};
	AllocatedImage InitTexture(const RenderSubsystemVK* renderSystem, const InitTextureParams& params);

	struct CreateDescriptorLayoutParams
	{
		vk::DescriptorSetLayoutCreateFlags flags;
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		std::vector<vk::DescriptorBindingFlags> bindingFlags;
	};
	vk::DescriptorSetLayout CreateDescriptorLayout(const vk::Device& device, const CreateDescriptorLayoutParams& params);

	void UpdateModelTransform(const Vector3f& position, const maths::Quat& orientation, const Vector3f& scale,
										glm::mat4& model);
}