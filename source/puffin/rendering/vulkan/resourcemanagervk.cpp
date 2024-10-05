#include "puffin/rendering/vulkan/resourcemanagervk.h"

#include <cmath>

#include "puffin/rendering/renderglobals.h"
#include "puffin/rendering/vulkan/unifiedgeometrybuffervk.h"
#include "puffin/rendering/vulkan/helpersvk.h"
#include "puffin/rendering/vulkan/rendersubsystemvk.h"

namespace puffin::rendering
{
	ResourceManagerVK::ResourceManagerVK(RenderSubsystemVK* renderSystem, uint8_t bufferedFrameCount)
		: mRenderSystem(renderSystem), mBufferedFrameCount(bufferedFrameCount)
	{
		mImageInstancesToDestroy.resize(mBufferedFrameCount);
		mBufferInstancesToDestroy.resize(mBufferedFrameCount);
	}

	ResourceManagerVK::~ResourceManagerVK()
	{
		for (const auto& [id, info] : mResourceInfo)
		{
			for (const auto& instanceID : info.instanceIDs)
			{
				if (info.type == ResourceType::Image)
				{
					DestroyImageInstanceInternal(instanceID);
				}
				else if (info.type == ResourceType::Buffer)
				{
					DestroyBufferInstanceInternal(instanceID);
				}
			}
		}

		mResourceInfo.clear();
		mResourceNameToID.clear();

		mImageInstancesToCreate.clear();
		mImageInstancesToDestroy.clear();

		mRenderSystem = nullptr;
	}

	void ResourceManagerVK::CreateAndUpdateResources()
	{
		UpdateSwapchainAndRenderRelativeResources();

		CreateResourcesInstances();
	}

	void ResourceManagerVK::DestroyResources()
	{
		DestroyResourcesInstances();
	}

	ResourceID ResourceManagerVK::CreateOrUpdateAttachment(const AttachmentDescVK& desc)
	{
		ResourceID id = GenerateId();

		CreateOrUpdateAttachmentInternal(desc, id, "");

		return id;
	}

	ResourceID ResourceManagerVK::CreateOrUpdateAttachment(const AttachmentDescVK& desc, ResourceID id)
	{
		CreateOrUpdateAttachmentInternal(desc, id, "");

		return id;
	}

	ResourceID ResourceManagerVK::CreateOrUpdateAttachment(const AttachmentDescVK& desc, const std::string& name)
	{
		ResourceID id;

		if (mResourceNameToID.find(name) != mResourceNameToID.end())
		{
			id = mResourceNameToID.at(name);
		}
		else
		{
			id = GenerateId();
		}

		CreateOrUpdateAttachmentInternal(desc, id, name);

		return id;
	}

	ResourceID ResourceManagerVK::CreateOrUpdateImage(const ImageDescVK& desc)
	{
		ResourceID id = GenerateId();

		CreateOrUpdateImageInternal(desc, id, "");

		return id;
	}

	ResourceID ResourceManagerVK::CreateOrUpdateImage(const ImageDescVK& desc, ResourceID id)
	{
		CreateOrUpdateImageInternal(desc, id, "");

		return id;
	}

	ResourceID ResourceManagerVK::CreateOrUpdateImage(const ImageDescVK& desc, const std::string& name)
	{
		ResourceID id;

		if (mResourceNameToID.find(name) != mResourceNameToID.end())
		{
			id = mResourceNameToID.at(name);
		}
		else
		{
			id = GenerateId();
		}

		CreateOrUpdateImageInternal(desc, id, name);

		return id;
	}

	ResourceID ResourceManagerVK::CreateOrUpdateBuffer(const BufferDescVK& desc)
	{
		ResourceID id = GenerateId();

		CreateOrUpdateBufferInternal(desc, id, "");

		return id;
	}

	ResourceID ResourceManagerVK::CreateOrUpdateBuffer(const BufferDescVK& desc, ResourceID id)
	{
		CreateOrUpdateBufferInternal(desc, id, "");

		return id;
	}

	ResourceID ResourceManagerVK::CreateOrUpdateBuffer(const BufferDescVK& desc, const std::string& name)
	{
		ResourceID id = GenerateId();

		if (mResourceNameToID.find(name) != mResourceNameToID.end())
		{
			id = mResourceNameToID.at(name);
		}
		else
		{
			id = GenerateId();
		}

		CreateOrUpdateBufferInternal(desc, id, name);

		return id;
	}

	void ResourceManagerVK::DestroyResource(ResourceID id)
	{
		DestroyResourceInternal(id);
	}

	void ResourceManagerVK::DestroyResource(const std::string& name)
	{
		if (mResourceNameToID.find(name) != mResourceNameToID.end())
		{
			DestroyResourceInternal(mResourceNameToID.at(name));
		}
	}

	bool ResourceManagerVK::IsResourceValid(const std::string& name) const
	{
		return mResourceNameToID.find(name) != mResourceNameToID.end() && IsResourceValid(mResourceNameToID.at(name));
	}

	bool ResourceManagerVK::IsResourceValid(ResourceID id) const
	{
		return mResourceInfo.find(id) != mResourceInfo.end();
	}

	AllocatedImage& ResourceManagerVK::GetImage(ResourceID id)
	{
		assert(IsResourceValid(id) && "ResourceManagerVK::GetImage -  No image with id exists");

		const auto& resourceInfo = mResourceInfo.at(id);

		ResourceID instanceID;
		if (resourceInfo.persistent)
		{
			instanceID = resourceInfo.instanceIDs[0];
		}
		else
		{
			instanceID = resourceInfo.instanceIDs[mRenderSystem->GetCurrentFrameIdx()];
		}

		return mImageInstances.at(instanceID);
	}

	AllocatedImage& ResourceManagerVK::GetImage(const std::string& name)
	{
		assert(mResourceNameToID.find(name) != mResourceNameToID.end() && "ResourceManagerVK::GetImage -  No image with name %s exists", name.c_str());

		return GetImage(mResourceNameToID.at(name));
	}

	AllocatedBuffer& ResourceManagerVK::GetBuffer(ResourceID id)
	{
		assert(IsResourceValid(id) && "ResourceManagerVK::GetBuffer -  No buffer with id %s exists", std::to_string(id).c_str());

		const auto& resourceInfo = mResourceInfo.at(id);

		ResourceID instanceID;
		if (resourceInfo.persistent)
		{
			instanceID = resourceInfo.instanceIDs[0];
		}
		else
		{
			instanceID = resourceInfo.instanceIDs[mRenderSystem->GetCurrentFrameIdx()];
		}

		return mBufferInstances.at(instanceID);
	}

	AllocatedBuffer& ResourceManagerVK::GetBuffer(const std::string& name)
	{
		assert(mResourceNameToID.find(name) != mResourceNameToID.end() && "ResourceManagerVK::GetBuffer -  No buffer with name %s exists", name.c_str());

		return GetBuffer(mResourceNameToID.at(name));
	}

	void ResourceManagerVK::NotifySwapchainResized()
	{
		mSwapchainResized = true;
	}

	void ResourceManagerVK::NotifyRenderExtentResized()
	{
		mRenderExtentResized = true;
	}

	void ResourceManagerVK::UpdateSwapchainAndRenderRelativeResources()
	{
		if (mSwapchainResized)
		{
			for (const auto& id : mSwapchainRelativeAttachments)
			{
				const auto& attachmentDesc = mAttachmentDescs.at(id);

				CreateOrUpdateAttachmentInternal(attachmentDesc, id, "");
			}

			mSwapchainResized = false;
		}

		if (mRenderExtentResized)
		{
			for (const auto& id : mRenderRelativeAttachments)
			{
				const auto& attachmentDesc = mAttachmentDescs.at(id);

				CreateOrUpdateAttachmentInternal(attachmentDesc, id, "");
			}

			mRenderExtentResized = false;
		}
	}

	void ResourceManagerVK::CreateResourcesInstances()
	{
		for (auto& [id, desc] : mImageInstancesToCreate)
		{
			CreateImageInstanceInternal(id, desc);
		}

		mImageInstancesToCreate.clear();

		for (auto& [id, desc] : mBufferInstancesToCreate)
		{
			CreateBufferInstanceInternal(id, desc);
		}

		mBufferInstancesToCreate.clear();
	}

	void ResourceManagerVK::DestroyResourcesInstances()
	{
		const auto frameIdx = mRenderSystem->GetCurrentFrameIdx();

		for (const auto& id : mImageInstancesToDestroy[frameIdx])
		{
			DestroyImageInstanceInternal(id);
		}

		mImageInstancesToDestroy[frameIdx].clear();

		for (const auto& id : mBufferInstancesToDestroy[frameIdx])
		{
			DestroyBufferInstanceInternal(id);
		}

		mBufferInstancesToDestroy[frameIdx].clear();
	}

	void ResourceManagerVK::CreateOrUpdateAttachmentInternal(const AttachmentDescVK& desc, ResourceID id,
	                                                         const std::string& name)
	{
		vk::Extent3D extent;
		if (desc.imageSize == ImageSizeVK::SwapchainRelative
			|| desc.imageSize == ImageSizeVK::RenderExtentRelative)
		{
			CalculateImageExtent(desc.imageSize, extent, desc.widthMult, desc.heightMult);

			if (desc.imageSize == ImageSizeVK::SwapchainRelative)
			{
				mSwapchainRelativeAttachments.emplace(id);
			}

			if (desc.imageSize == ImageSizeVK::RenderExtentRelative)
			{
				mRenderRelativeAttachments.emplace(id);
			}
		}
		else
		{
			extent.width = desc.width;
			extent.height = desc.height;
			extent.depth = 1;
		}

		ImageDescVK imageCreateParams;
		imageCreateParams.info = { {}, vk::ImageType::e2D, desc.format, extent, 1, 1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, {vk::ImageUsageFlagBits::eSampled } };

		constexpr vk::ImageSubresourceRange subresourceRange{ {}, 0, 1, 0, 1 };

		imageCreateParams.viewInfo = { {}, {}, vk::ImageViewType::e2D, desc.format, {}, subresourceRange };

		if (desc.type == AttachmentTypeVK::Color)
		{
			imageCreateParams.info.usage |= vk::ImageUsageFlagBits::eColorAttachment;
			imageCreateParams.viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		}
		else if(desc.type == AttachmentTypeVK::Depth)
		{
			imageCreateParams.info.usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
			imageCreateParams.viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
		}

		mAttachmentDescs.emplace(id, desc);

		CreateOrUpdateImageInternal(imageCreateParams, id, name);
	}

	void ResourceManagerVK::CreateOrUpdateImageInternal(const ImageDescVK& desc, ResourceID id, const std::string& name)
	{
		if (IsResourceValid(id))
		{
			// If resource is valid, queue existing resource instances to be destroyed and queue new ones to be created
			auto& resourceInfo = mResourceInfo.at(id);

			if (resourceInfo.persistent)
			{
				mImageInstancesToDestroy[mBufferedFrameCount - 1].emplace(resourceInfo.instanceIDs[0]);

				resourceInfo.instanceIDs[mBufferedFrameCount - 1] = GenerateId();
			}
			else
			{
				for (int i = 0; i < mBufferedFrameCount; ++i)
				{
					mImageInstancesToDestroy[i].emplace(resourceInfo.instanceIDs[i]);
					resourceInfo.instanceIDs[i] = GenerateId();
				}
			}
		}
		else
		{
			// If resource is not valid, add new resource info and queue up new instances to be created
			mResourceInfo.emplace(id, ResourceInfo{});

			auto& resourceInfo = mResourceInfo.at(id);
			resourceInfo.id = id;
			resourceInfo.type = ResourceType::Image;
			resourceInfo.persistent = desc.persistent;

			if (resourceInfo.persistent)
			{
				resourceInfo.instanceIDs.push_back(GenerateId());
			}
			else
			{
				for (int i = 0; i < mBufferedFrameCount; ++i)
				{
					resourceInfo.instanceIDs.push_back(GenerateId());
				}
			}

			if (!name.empty())
			{
				resourceInfo.name = name;

				if (mResourceNameToID.find(name) == mResourceNameToID.end())
					mResourceNameToID.emplace(name, id);
			}
		}

		const auto& resourceInfo = mResourceInfo.at(id);
		for (auto instanceID : resourceInfo.instanceIDs)
		{
			mImageInstancesToCreate.emplace_back(instanceID, desc);
		}
	}

	void ResourceManagerVK::CreateOrUpdateBufferInternal(const BufferDescVK& desc, ResourceID id,
		const std::string& name)
	{
		if (IsResourceValid(id))
		{
			// If resource is valid, queue existing resource instances to be destroyed and queue new ones to be created
			auto& resourceInfo = mResourceInfo.at(id);

			if (resourceInfo.persistent)
			{
				mBufferInstancesToDestroy[mBufferedFrameCount - 1].emplace(resourceInfo.instanceIDs[0]);

				resourceInfo.instanceIDs[mBufferedFrameCount - 1] = GenerateId();
			}
			else
			{
				for (int i = 0; i < mBufferedFrameCount; ++i)
				{
					mBufferInstancesToDestroy[i].emplace(resourceInfo.instanceIDs[i]);
					resourceInfo.instanceIDs[i] = GenerateId();
				}
			}
		}
		else
		{
			// If resource is not valid, add new resource info and queue up new instances to be created
			mResourceInfo.emplace(id, ResourceInfo{});

			auto& resourceInfo = mResourceInfo.at(id);
			resourceInfo.id = id;
			resourceInfo.type = ResourceType::Buffer;
			resourceInfo.persistent = desc.persistent;

			if (resourceInfo.persistent)
			{
				resourceInfo.instanceIDs.push_back(GenerateId());
			}
			else
			{
				for (int i = 0; i < mBufferedFrameCount; ++i)
				{
					resourceInfo.instanceIDs.push_back(GenerateId());
				}
			}

			if (!name.empty())
			{
				resourceInfo.name = name;

				if (mResourceNameToID.find(name) == mResourceNameToID.end())
					mResourceNameToID.emplace(name, id);
			}
		}

		mBufferDescs.emplace(id, desc);

		const auto& resourceInfo = mResourceInfo.at(id);
		for (auto instanceID : resourceInfo.instanceIDs)
		{
			mBufferInstancesToCreate.emplace_back(instanceID, desc);
		}
	}

	void ResourceManagerVK::DestroyResourceInternal(ResourceID id)
	{
		if (IsResourceValid(id))
		{
			const auto& resourceInfo = mResourceInfo.at(id);

			if (resourceInfo.type == ResourceType::Image)
			{
				for (int i = 0; i < mBufferedFrameCount; ++i)
				{
					mImageInstancesToDestroy[i].emplace(resourceInfo.instanceIDs[i]);
				}

				if (mSwapchainRelativeAttachments.find(id) != mSwapchainRelativeAttachments.end())
					mSwapchainRelativeAttachments.erase(id);

				if (mRenderRelativeAttachments.find(id) != mRenderRelativeAttachments.end())
					mRenderRelativeAttachments.erase(id);
			}
			else if (resourceInfo.type == ResourceType::Buffer)
			{
				for (int i = 0; i < mBufferedFrameCount; ++i)
				{
					mBufferInstancesToDestroy[i].emplace(resourceInfo.instanceIDs[i]);
				}
			}

			mResourceInfo.erase(id);
		}
	}

	void ResourceManagerVK::CreateImageInstanceInternal(ResourceID instanceID, const ImageDescVK& desc)
	{
		util::CreateImageParams createImageParams;
		createImageParams.imageInfo = desc.info;
		createImageParams.imageViewInfo = desc.viewInfo;

		mImageInstances.emplace(instanceID, util::CreateImage(mRenderSystem, createImageParams));
	}

	void ResourceManagerVK::DestroyImageInstanceInternal(ResourceID instanceID)
	{
		if (mImageInstances.find(instanceID) == mImageInstances.end())
			return;

		const auto& allocImage = mImageInstances.at(instanceID);

		mRenderSystem->GetDevice().destroyImageView(allocImage.imageView);
		mRenderSystem->GetAllocator().destroyImage(allocImage.image, allocImage.allocation);

		mImageInstances.erase(instanceID);
	}

	void ResourceManagerVK::CreateBufferInstanceInternal(ResourceID instanceID, const BufferDescVK& desc)
	{
		util::CreateBufferParams createBufferParams;
		createBufferParams.allocSize = desc.size;
		createBufferParams.bufferUsage = desc.usage;
		createBufferParams.memoryUsage = desc.memoryUsage;
		createBufferParams.allocFlags = desc.allocFlags;
		createBufferParams.requiredFlags = desc.propFlags;

		mBufferInstances.emplace(instanceID, util::CreateBuffer(mRenderSystem->GetAllocator(), createBufferParams));
	}

	void ResourceManagerVK::DestroyBufferInstanceInternal(ResourceID instanceID)
	{
		if (mBufferInstances.find(instanceID) == mBufferInstances.end())
			return;

		const auto& allocBuffer = mBufferInstances.at(instanceID);

		mRenderSystem->GetAllocator().destroyBuffer(allocBuffer.buffer, allocBuffer.allocation);

		mBufferInstances.erase(instanceID);
	}

	void ResourceManagerVK::CalculateImageExtent(ImageSizeVK imageSize, vk::Extent3D& extent, float widthMult, float heightMult) const
	{
		// Update Width/Height of Swapchain/Render Relative
		vk::Extent2D extent2D;

		if (imageSize == ImageSizeVK::SwapchainRelative)
		{
			extent2D = mRenderSystem->GetSwapchainExtent();
		}
		else
		{
			extent2D = mRenderSystem->GetRenderExtent();
		}

		extent.width = static_cast<uint32_t>(std::ceil(extent2D.width * widthMult));
		extent.height = static_cast<uint32_t>(std::ceil(extent2D.height * heightMult));
		extent.depth = 1;
	}
}
