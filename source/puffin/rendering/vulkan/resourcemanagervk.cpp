#include "puffin/rendering/vulkan/resourcemanagervk.h"

#include "puffin/rendering/renderglobals.h"
#include "puffin/rendering/vulkan/unifiedgeometrybuffervk.h"
#include "puffin/rendering/vulkan/helpersvk.h"
#include "puffin/rendering/vulkan/rendersubsystemvk.h"

namespace puffin::rendering
{
	ResourceManagerVK::ResourceManagerVK(RenderSubsystemVK* renderSystem)
		: mRenderSystem(renderSystem)
	{
		mUnifiedGeometryBuffer = new UnifiedGeometryBuffer(mRenderSystem);
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

	UnifiedGeometryBuffer* ResourceManagerVK::GeometryBuffer() const
	{
		return mUnifiedGeometryBuffer;
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
