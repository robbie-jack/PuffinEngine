#pragma once

#include <memory>

#include "puffin/rendering/vulkan/typesvk.h"
#include "puffin/types/storage/mappedvector.h"
#include "puffin/types/uuid.h"
#include "puffin/rendering/resourceid.h"

namespace puffin
{
	namespace assets
	{
		class StaticMeshAsset;
	}
}

namespace puffin::rendering
{
	class RenderSubsystemVK;
	class UnifiedGeometryBuffer;

	class ResourceManagerVK
	{
	public:

		explicit ResourceManagerVK(RenderSubsystemVK* renderSystem);
		~ResourceManagerVK();

		void AddStaticMesh(const std::shared_ptr<assets::StaticMeshAsset>& staticMesh);

		ResourceID AddImages(const ImageDesc& imageDesc, uint8_t imageCount);

		void DestroyImages(ResourceID id);

		void UpdateImage(ResourceID id, const ImageDesc& imageDesc, uint8_t imageIdx);
		void UpdateImages(ResourceID id, const ImageDesc& imageDesc);

		AllocatedImage& GetImage(ResourceID id, uint8_t idx = 0);
		[[nodiscard]] bool IsImageValid(ResourceID id) const;
		[[nodiscard]] bool IsImageValid(ResourceID id, uint8_t idx) const;
		[[nodiscard]] size_t GetImageCount(ResourceID id) const;

		UnifiedGeometryBuffer* GeometryBuffer() const;

	private:

		void CreateImageInternal(ResourceID id, const ImageDesc& imageDesc, uint8_t idx = 0);
		void DestroyImageInternal(ResourceID id, uint8_t idx = 0);
		void UpdateImageInternal(ResourceID id, const ImageDesc& imageDesc, uint8_t idx = 0);

		RenderSubsystemVK* mRenderSystem = nullptr;
		UnifiedGeometryBuffer* mUnifiedGeometryBuffer = nullptr;

		MappedVector<ResourceID, std::vector<AllocatedImage>> mImages;

		

	};
}
