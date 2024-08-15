#pragma once

#include <functional>
#include <memory>

#include "puffin/rendering/vulkan/typesvk.h"

namespace puffin::rendering
{
	class RenderSubystemVK;
}

namespace puffin::rendering::util
{
	void immediate_submit(const RenderSubystemVK* render_system, std::function<void(VkCommandBuffer cmd)>&& function);

	void copy_data_between_buffers(const RenderSubystemVK* render_system, vk::Buffer src_buffer, vk::Buffer dst_buffer,
	                               uint32_t data_size, uint32_t src_offset = 0, uint32_t dst_offset = 0);

	void copy_cpu_data_into_gpu_buffer(const RenderSubystemVK* render_system, const AllocatedBuffer& dst_buffer, uint32_t data_size,
	                                   const void* data, uint32_t src_offset = 0, uint32_t dst_offset = 0);

	AllocatedBuffer create_buffer(const vma::Allocator& allocator, size_t alloc_size, vk::BufferUsageFlags usage, vma::MemoryUsage memory_usage, vma::
		AllocationCreateFlags alloc_flags = {}, vk::MemoryPropertyFlags required_flags = {});

	AllocatedBuffer init_vertex_buffer(const RenderSubystemVK* render_system, const void* vertex_data, const size_t num_vertices, const size_t vertex_size);

	AllocatedBuffer init_index_buffer(const RenderSubystemVK* render_system, const void* index_data, const size_t num_indices, const size_t index_size);

	AllocatedImage create_image(const RenderSubystemVK* render_system, const vk::ImageCreateInfo& image_info, vk::ImageViewCreateInfo image_view_info);

	AllocatedImage create_color_image(const RenderSubystemVK* render_system, vk::Extent3D extent, vk::Format format);
	AllocatedImage create_depth_image(const RenderSubystemVK* render_system, vk::Extent3D extent, vk::Format format);

	AllocatedImage init_texture(const RenderSubystemVK* render_system, const void* pixel_data, uint32_t width, uint32_t height, vk::DeviceSize
	                            size, vk::Format format);
}