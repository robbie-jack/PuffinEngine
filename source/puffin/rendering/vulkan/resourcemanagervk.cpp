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
		if (mImages.Size() > 0)
		{
			for (auto& images : mImages)
			{
				for (auto& alloc_image : images)
				{
					mRenderSystem->GetDevice().destroyImageView(alloc_image.imageView);
					mRenderSystem->GetAllocator().destroyImage(alloc_image.image, alloc_image.allocation);
				}
			}

			mImages.Clear();
		}

		mRenderSystem = nullptr;
	}

	void ResourceManagerVK::AddStaticMesh(const std::shared_ptr<assets::StaticMeshAsset>& staticMesh)
	{
		mUnifiedGeometryBuffer->AddStaticMesh(staticMesh);
	}

	ResourceID ResourceManagerVK::AddImage(const ImageDescVK& imageDesc)
	{
		ResourceID id = GenerateId();

		AddImageInternal(imageDesc, id, "");

		return id;
	}

	ResourceID ResourceManagerVK::AddImage(const ImageDescVK& imageDesc, ResourceID id)
	{
		AddImageInternal(imageDesc, id, "");

		return id;
	}

	ResourceID ResourceManagerVK::AddImage(const ImageDescVK& imageDesc, const std::string& name)
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

		AddImageInternal(imageDesc, id, name);

		return id;
	}

	ResourceID ResourceManagerVK::AddBuffer(const BufferDescVK& bufferDesc, const std::string& name)
	{
		ResourceID id = GenerateId();



		return id;
	}

	void ResourceManagerVK::Update()
	{
		CreateResourcesInstances();

		DestroyResourcesInstances();
	}

	ResourceID ResourceManagerVK::AddImages(const ImageDesc& imageDesc, uint8_t imageCount)
	{
		ResourceID id = GenerateId();

		mImages.Emplace(id, std::vector<AllocatedImage>());
		mImages.At(id).resize(imageCount);

		for (int i = 0; i < imageCount; ++i)
		{
			CreateImageInternal(id, imageDesc, i);
		}

		return id;
	}

	void ResourceManagerVK::DestroyImages(ResourceID id)
	{
		if (mImages.Contains(id))
		{
			for (int i = 0; i < mImages.At(id).size(); ++i)
			{
				DestroyImageInternal(id, i);
			}

			mImages.Erase(id);
		}
	}

	void ResourceManagerVK::UpdateImage(ResourceID id, const ImageDesc& imageDesc, uint8_t imageIdx)
	{
		assert(mImages.Contains(id) && "ResourceManagerVK::UpdateImage - Atempting to get image with invalid resource id");

		if (mImages.Contains(id))
		{
			UpdateImageInternal(id, imageDesc, imageIdx);
		}
	}

	void ResourceManagerVK::UpdateImages(ResourceID id, const ImageDesc& imageDesc)
	{
		assert(mImages.Contains(id) && "ResourceManagerVK::UpdateImage - Atempting to get image with invalid resource id");

		if (mImages.Contains(id))
		{
			for (int i = 0; i < mImages.At(id).size(); ++i)
			{
				UpdateImageInternal(id, imageDesc, i);
			}
		}
	}

	AllocatedImage& ResourceManagerVK::GetImage(ResourceID id, uint8_t idx)
	{
		assert(mImages.Contains(id) && "ResourceManagerVK::GetImage - Atempting to get image with invalid resource id");

		if (mImages.At(id).size() == 1)
		{
			return mImages.At(id)[0];
		}

		return mImages.At(id)[idx];
	}

	bool ResourceManagerVK::IsImageValid(ResourceID id) const
	{
		return mImages.Contains(id);
	}

	bool ResourceManagerVK::IsImageValid(ResourceID id, uint8_t idx) const
	{
		return mImages.At(id).size() > idx;
	}

	size_t ResourceManagerVK::GetImageCount(ResourceID id) const
	{
		assert(mImages.Contains(id) && "ResourceManagerVK::GetImageCount - Atempting to get image with invalid resource id");

		return mImages.At(id).size();
	}

	bool ResourceManagerVK::IsResourceValid(const std::string& name) const
	{
		return mResourceNameToID.find(name) != mResourceNameToID.end() && IsResourceValid(mResourceNameToID.at(name));
	}

	bool ResourceManagerVK::IsResourceValid(ResourceID id) const
	{
		return mResourceInfo.find(id) != mResourceInfo.end();
	}

	UnifiedGeometryBuffer* ResourceManagerVK::GeometryBuffer() const
	{
		return mUnifiedGeometryBuffer;
	}

	void ResourceManagerVK::CreateResourcesInstances()
	{
		for (auto& [id, imageDesc] : mImageInstancesToCreate)
		{
			CreateImageInstanceInternal(id, imageDesc);
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

	void ResourceManagerVK::AddImageInternal(const ImageDescVK& imageDesc, ResourceID id, const std::string& name)
	{
		if (mResourceInfo.find(id) == mResourceInfo.end())
		{
			mResourceInfo.emplace(id, ResourceInfo{});

			auto& resourceInfo = mResourceInfo.at(id);
			resourceInfo.id = id;
			resourceInfo.type = ResourceType::Image;
			resourceInfo.persistent = imageDesc.persistent;

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
			mImageInstancesToCreate.emplace(instanceID, imageDesc);
		}
	}

	void ResourceManagerVK::UpdateImageInternal(const ImageDescVK& imageDesc, ResourceID id)
	{
		if (IsResourceValid(id))
		{
			auto& resourceInfo = mResourceInfo.at(id);

			uint8_t instanceIdx;

			if (resourceInfo.persistent)
			{
				instanceIdx = 0;
			}
			else
			{
				instanceIdx = mRenderSystem->GetCurrentFrameIdx();
			}

			mImageInstancesToDestroy[instanceIdx].emplace(resourceInfo.instanceIDs[instanceIdx]);

			resourceInfo.instanceIDs[instanceIdx] = GenerateId();
			mImageInstancesToCreate.emplace(resourceInfo.instanceIDs[instanceIdx], imageDesc);
		}
	}

	void ResourceManagerVK::CreateImageInstanceInternal(ResourceID instanceID, ImageDescVK& imageDesc)
	{
		util::CreateImageParams params;

		// Update Width/Height of Swapchain/Render Relative
		if (imageDesc.imageSizeType == ImageSizeVK::SwapchainRelative 
			|| imageDesc.imageSizeType == ImageSizeVK::RenderExtentRelative)
		{
			vk::Extent2D extent;

			if (imageDesc.imageSizeType == ImageSizeVK::SwapchainRelative)
			{
				extent = mRenderSystem->GetSwapchainExtent();
			}
			else
			{
				extent = mRenderSystem->GetRenderExtent();
			}

			imageDesc.width = static_cast<uint32_t>(std::ceil(extent.width * imageDesc.widthMult));
			imageDesc.height = static_cast<uint32_t>(std::ceil(extent.height * imageDesc.heightMult));
			imageDesc.depth = 0;
		}

		assert(imageDesc.width > 0 && "ResourceManagerVK::CreateImageInstanceInternal - Resource width was not greater than 0, which is invalid behaviour");

		vk::ImageType imageType = vk::ImageType::e1D;

		// Decide if image is 1D, 2D, or 3D
		if (imageDesc.height > 0)
		{
			if (imageDesc.depth > 0)
			{
				imageType = vk::ImageType::e3D;
			}
			else
			{
				imageType = vk::ImageType::e2D;
			}
		}


	}

	void ResourceManagerVK::DestroyImageInstanceInternal(ResourceID instanceID)
	{
		const auto& allocImage = mAllocImageInstances.at(instanceID);

		mRenderSystem->GetDevice().destroyImageView(allocImage.imageView);
		mRenderSystem->GetAllocator().destroyImage(allocImage.image, allocImage.allocation);

		mAllocImageInstances.erase(instanceID);
		mImageDescInstances.erase(instanceID);
	}

	void ResourceManagerVK::CreateImageInternal(ResourceID id, const ImageDesc& imageDesc, uint8_t idx)
	{
		const vk::Extent3D extent = { imageDesc.width, imageDesc.height, imageDesc.depth };

		util::CreateFormattedImageParams params;
		params.extent = extent;
		params.format = imageDesc.format;

		if (imageDesc.imageType == ImageType::Color)
		{
			mImages.At(id)[idx] = util::CreateColorImage(mRenderSystem, params);
		}
		else
		{
			mImages.At(id)[idx] = util::CreateDepthImage(mRenderSystem, params);
		}
	}

	void ResourceManagerVK::DestroyImageInternal(ResourceID id, uint8_t idx)
	{
		const auto& allocImage = mImages.At(id)[idx];

		mRenderSystem->GetDevice().destroyImageView(allocImage.imageView);
		mRenderSystem->GetAllocator().destroyImage(allocImage.image, allocImage.allocation);
	}

	void ResourceManagerVK::UpdateImageInternal(ResourceID id, const ImageDesc& imageDesc, uint8_t idx)
	{
		DestroyImageInternal(id, idx);
		CreateImageInternal(id, imageDesc, idx);
	}
}
