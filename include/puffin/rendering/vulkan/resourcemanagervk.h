#pragma once

#include <memory>
#include <unordered_set>

#include "puffin/rendering/vulkan/typesvk.h"
#include "puffin/types/storage/mappedvector.h"
#include "puffin/types/uuid.h"
#include "puffin/rendering/resourceid.h"
#include "puffin/rendering/vulkan/resourcedescvk.h"

namespace puffin::rendering
{
	struct BufferDescVK;
	struct ImageDescVK;
	class RenderSubsystemVK;
	class UnifiedGeometryBufferVK;

	class ResourceManagerVK
	{
	public:

		explicit ResourceManagerVK(RenderSubsystemVK* renderSystem, uint8_t bufferedFrameCount);
		~ResourceManagerVK();

		/*
		 * Create & Update new and existing resources
		 */
		void CreateAndUpdateResources();

		/*
		 * Destroy old resources that are no longer needed
		 */
		void DestroyResources();
		
		[[nodiscard]] ResourceID CreateOrUpdateAttachment(const AttachmentDescVK& desc);
		[[nodiscard]] ResourceID CreateOrUpdateAttachment(const AttachmentDescVK& desc, ResourceID id);
		[[nodiscard]] ResourceID CreateOrUpdateAttachment(const AttachmentDescVK& desc, const std::string& name);
		
		[[nodiscard]] ResourceID CreateOrUpdateImage(const ImageDescVK& desc);
		[[nodiscard]] ResourceID CreateOrUpdateImage(const ImageDescVK& desc, ResourceID id);
		[[nodiscard]] ResourceID CreateOrUpdateImage(const ImageDescVK& desc, const std::string& name);

		[[nodiscard]] ResourceID CreateOrUpdateBuffer(const BufferDescVK& desc);
		[[nodiscard]] ResourceID CreateOrUpdateBuffer(const BufferDescVK& desc, ResourceID id);
		[[nodiscard]] ResourceID CreateOrUpdateBuffer(const BufferDescVK& desc, const std::string& name);

		[[nodiscard]] ResourceID CreateOrUpdateDescriptorLayout(const DescriptorLayoutDescVK& desc);
		[[nodiscard]] ResourceID CreateOrUpdateDescriptorLayout(const DescriptorLayoutDescVK& desc, ResourceID id);
		[[nodiscard]] ResourceID CreateOrUpdateDescriptorLayout(const DescriptorLayoutDescVK& desc, const std::string& name);

		void DestroyResource(ResourceID id);
		void DestroyResource(const std::string& name);

		[[nodiscard]] bool IsResourceValid(const std::string& name) const;
		[[nodiscard]] bool IsResourceValid(ResourceID id) const;

		AllocatedImage& GetImage(ResourceID id);
		AllocatedImage& GetImage(const std::string& name);

		AllocatedBuffer& GetBuffer(ResourceID id);
		AllocatedBuffer& GetBuffer(const std::string& name);

		vk::DescriptorSetLayout& GetDescriptorLayout(ResourceID id);
		vk::DescriptorSetLayout& GetDescriptorLayout(const std::string& name);

		void NotifySwapchainResized();
		void NotifyRenderExtentResized();

	private:

		void UpdateSwapchainAndRenderRelativeResources();
		void CreateResourceInstances();
		void DestroyResourceInstances();

		void CreateOrUpdateAttachmentInternal(const AttachmentDescVK& desc, ResourceID id, const std::string& name);
		void CreateOrUpdateImageInternal(const ImageDescVK& desc, ResourceID id, const std::string& name);
		void CreateOrUpdateBufferInternal(const BufferDescVK& desc, ResourceID id, const std::string& name);
		void CreateOrUpdateDescriptorLayoutInternal(const DescriptorLayoutDescVK& desc, ResourceID id, const std::string& name);

		void DestroyResourceInternal(ResourceID id);

		void CreateImageInstanceInternal(ResourceID instanceID, const ImageDescVK& desc);
		void DestroyImageInstanceInternal(ResourceID instanceID);

		void CreateBufferInstanceInternal(ResourceID instanceID, const BufferDescVK& desc);
		void DestroyBufferInstanceInternal(ResourceID instanceID);

		void CreateDescriptorLayoutInstanceInternal(ResourceID instanceID, const DescriptorLayoutDescVK& desc);
		void DestroyDescriptorLayoutInstanceInternal(ResourceID instanceID);

		void CalculateImageExtent(ImageSizeVK imageSize, vk::Extent3D& extent, float widthMult = 1.0f, float heightMult = 1.0f) const;

		enum class ResourceType
		{
			Image = 0,
			Buffer,
			DescriptorLayout
		};

		struct ResourceInfo
		{
			std::string name = "";
			ResourceID id = gInvalidID;
			ResourceType type;
			bool persistent = false;
			std::vector<uint8_t> instanceIDs; // IDs of individual instances of this resource
		};

		RenderSubsystemVK* mRenderSystem = nullptr;

		bool mSwapchainResized = false;
		bool mRenderExtentResized = false;

		uint8_t mBufferedFrameCount = 0;
		std::unordered_map<ResourceID, ResourceInfo> mResourceInfo;
		std::unordered_map<std::string, ResourceID> mResourceNameToID;

		/* 
		 * Set of attachments which are swapchain relative and should be resized when
		 * swapchain size changes
		 */
		std::unordered_set<ResourceID> mSwapchainRelativeAttachments;

		/*
		 * Set of attachments which are render extent relative and should be resized when
		 * render size changes
		 */
		std::unordered_set<ResourceID> mRenderRelativeAttachments;

		std::unordered_map<ResourceID, AttachmentDescVK> mAttachmentDescs;
		std::unordered_map<ResourceID, BufferDescVK> mBufferDescs;
		std::unordered_map<ResourceID, DescriptorLayoutDescVK> mDescriptorLayoutDescs;

		std::unordered_map<ResourceID, AllocatedImage> mImageInstances;
		std::unordered_map<ResourceID, AllocatedBuffer> mBufferInstances;
		std::unordered_map<ResourceID, vk::DescriptorSetLayout> mDescriptorLayoutInstances;

		std::vector<std::pair<ResourceID, ImageDescVK>> mImageInstancesToCreate;
		std::vector<std::unordered_set<ResourceID>> mImageInstancesToDestroy;

		std::vector<std::pair<ResourceID, BufferDescVK>> mBufferInstancesToCreate;
		std::vector<std::unordered_set<ResourceID>> mBufferInstancesToDestroy;

		std::vector<std::pair<ResourceID, DescriptorLayoutDescVK>> mDescriptorLayoutInstancesToCreate;
		std::vector<ResourceID> mDescriptorLayoutInstancesToDestroy;
	};
}
