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
		mUnifiedGeometryBuffer = new UnifiedGeometryBuffer(mRenderSystem);
		mImageInstancesToDestroy.resize(mBufferedFrameCount);
	}

	ResourceManagerVK::~ResourceManagerVK()
	{
		for (const auto& [id, info] : mResourceInfo)
		{
			for (const auto& instanceID : info.instanceIDs)
			{
				DestroyImageInstanceInternal(instanceID);
			}
		}

		mResourceInfo.clear();
		mResourceNameToID.clear();

		mImageInstancesToCreate.clear();
		mImageInstancesToDestroy.clear();

		delete mUnifiedGeometryBuffer;
		mUnifiedGeometryBuffer = nullptr;

		mRenderSystem = nullptr;
	}

	void ResourceManagerVK::AddStaticMesh(const std::shared_ptr<assets::StaticMeshAsset>& staticMesh)
	{
		mUnifiedGeometryBuffer->AddStaticMesh(staticMesh);
	}

	ResourceID ResourceManagerVK::CreateOrUpdateAttachment(const AttachmentParams& params)
	{
		ResourceID id = GenerateId();

		CreateOrUpdateAttachmentInternal(params, id, "");

		return id;
	}

	ResourceID ResourceManagerVK::CreateOrUpdateAttachment(const AttachmentParams& params, ResourceID id)
	{
		CreateOrUpdateAttachmentInternal(params, id, "");

		return id;
	}

	ResourceID ResourceManagerVK::CreateOrUpdateAttachment(const AttachmentParams& params, const std::string& name)
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

		CreateOrUpdateAttachmentInternal(params, id, name);

		return id;
	}

	ResourceID ResourceManagerVK::CreateOrUpdateImage(const ResourceManagerVK::ImageCreateParams& params)
	{
		ResourceID id = GenerateId();

		CreateOrUpdateImageInternal(params, id, "");

		return id;
	}

	ResourceID ResourceManagerVK::CreateOrUpdateImage(const ResourceManagerVK::ImageCreateParams& params, ResourceID id)
	{
		CreateOrUpdateImageInternal(params, id, "");

		return id;
	}

	ResourceID ResourceManagerVK::CreateOrUpdateImage(const ResourceManagerVK::ImageCreateParams& params, const std::string& name)
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

		CreateOrUpdateImageInternal(params, id, name);

		return id;
	}

	ResourceID ResourceManagerVK::CreateOrUpdateBuffer(const BufferDescVK& bufferDesc, const std::string& name)
	{
		ResourceID id = GenerateId();



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

	void ResourceManagerVK::Update()
	{
		CreateResourcesInstances();

		DestroyResourcesInstances();
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
		assert(IsResourceValid(id) && "ResourceManagerVK::GetImage -  No image with id %s exists", std::to_string(id).c_str());

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

		return mInstanceImages.at(instanceID);
	}

	AllocatedImage& ResourceManagerVK::GetImage(const std::string& name)
	{
		assert(mResourceNameToID.find(name) != mResourceNameToID.end() && "ResourceManagerVK::GetImage -  No image with name %s exists", name.c_str());

		return GetImage(mResourceNameToID.at(name));
	}

	UnifiedGeometryBuffer* ResourceManagerVK::GeometryBuffer() const
	{
		return mUnifiedGeometryBuffer;
	}

	void ResourceManagerVK::CreateResourcesInstances()
	{
		for (auto& [id, params] : mImageInstancesToCreate)
		{
			CreateImageInstanceInternal(id, params);
		}

		mImageInstancesToCreate.clear();
	}

	void ResourceManagerVK::DestroyResourcesInstances()
	{
		for (const auto& id : mImageInstancesToDestroy[mRenderSystem->GetCurrentFrameIdx()])
		{
			DestroyImageInstanceInternal(id);
		}

		mImageInstancesToDestroy[mRenderSystem->GetCurrentFrameIdx()].clear();
	}

	void ResourceManagerVK::CreateOrUpdateAttachmentInternal(const AttachmentParams& params, ResourceID id,
		const std::string& name)
	{
		vk::Extent3D extent;
		CalculateImageExtent(params.imageSize, extent, params.widthMult, params.heightMult);

		ImageCreateParams imageCreateParams;
		imageCreateParams.info = { {}, vk::ImageType::e2D, params.format, extent, 1, 1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, {vk::ImageUsageFlagBits::eSampled } };

		constexpr vk::ImageSubresourceRange subresourceRange{ {}, 0, 1, 0, 1 };

		imageCreateParams.viewInfo = { {}, {}, vk::ImageViewType::e2D, params.format, {}, subresourceRange };

		if (params.type == AttachmentType::Color)
		{
			imageCreateParams.info.usage |= vk::ImageUsageFlagBits::eColorAttachment;
			imageCreateParams.viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		}
		else if(params.type == AttachmentType::Depth)
		{
			imageCreateParams.info.usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
			imageCreateParams.viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
		}

		CreateOrUpdateImageInternal(imageCreateParams, id, name);
	}

	void ResourceManagerVK::CreateOrUpdateImageInternal(const ImageCreateParams& params, ResourceID id, const std::string& name)
	{
		if (IsResourceValid(id))
		{
			// If resource is valid, queue existing resource instances to be destroyed and queue new ones to be created
			auto& resourceInfo = mResourceInfo.at(id);

			if (resourceInfo.persistent)
			{
				mImageInstancesToDestroy[0].emplace(resourceInfo.instanceIDs[0]);

				resourceInfo.instanceIDs[0] = GenerateId();
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
			resourceInfo.persistent = params.persistent;

			if (resourceInfo.persistent)
			{
				resourceInfo.instanceIDs.push_back(GenerateId());
			}
			else
			{
				resourceInfo.instanceIDs.push_back(GenerateId());
				resourceInfo.instanceIDs.push_back(GenerateId());
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
			mImageInstancesToCreate.emplace_back(instanceID, params);
		}
	}

	void ResourceManagerVK::DestroyResourceInternal(ResourceID id)
	{
		if (IsResourceValid(id))
		{
			const auto& resourceInfo = mResourceInfo.at(id);

			for (int i = 0; i < mBufferedFrameCount; ++i)
			{
				mImageInstancesToDestroy[i].emplace(resourceInfo.instanceIDs[i]);
			}

			mResourceInfo.erase(id);
		}
	}

	void ResourceManagerVK::CreateImageInstanceInternal(ResourceID instanceID, const ImageCreateParams& params)
	{
		util::CreateImageParams createImageParams;
		createImageParams.imageInfo = params.info;
		createImageParams.imageViewInfo = params.viewInfo;

		mInstanceImages.emplace(instanceID, util::CreateImage(mRenderSystem, createImageParams));
		mInstanceImageDescs.emplace(instanceID, params);
	}

	void ResourceManagerVK::DestroyImageInstanceInternal(ResourceID instanceID)
	{
		const auto& allocImage = mInstanceImages.at(instanceID);

		mRenderSystem->GetDevice().destroyImageView(allocImage.imageView);
		mRenderSystem->GetAllocator().destroyImage(allocImage.image, allocImage.allocation);

		mInstanceImages.erase(instanceID);
		mInstanceImageDescs.erase(instanceID);
	}

	void ResourceManagerVK::CalculateImageExtent(ImageSizeVK imageSize, vk::Extent3D& extent, float widthMult, float heightMult) const
	{
		// Update Width/Height of Swapchain/Render Relative
		if (imageSize == ImageSizeVK::SwapchainRelative
			|| imageSize == ImageSizeVK::RenderExtentRelative)
		{
			vk::Extent2D extent2D;

			if (imageSize == ImageSizeVK::SwapchainRelative)
			{
				extent2D = mRenderSystem->GetSwapchainExtent();
			}
			else
			{
				extent2D = mRenderSystem->GetRenderExtent();
			}

			extent.width = static_cast<uint32_t>(std::ceil(extent.width * widthMult));
			extent.height = static_cast<uint32_t>(std::ceil(extent.height * heightMult));
			extent.depth = 1;
		}
	}
}
