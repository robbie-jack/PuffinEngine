#pragma once

#include <memory>
#include <unordered_set>

#include "puffin/rendering/vulkan/typesvk.h"
#include "puffin/types/storage/mappedvector.h"
#include "puffin/types/uuid.h"
#include "puffin/rendering/resourceid.h"
#include "rendergraph/imagedescvk.h"

namespace puffin
{
	namespace assets
	{
		class StaticMeshAsset;
	}
}

namespace puffin::rendering
{
	struct BufferDescVK;
	struct ImageDescVK;
	class RenderSubsystemVK;
	class UnifiedGeometryBuffer;

	enum class AttachmentType
	{
		Color,
		Depth
	};

	class ResourceManagerVK
	{
	public:

		explicit ResourceManagerVK(RenderSubsystemVK* renderSystem, uint8_t bufferedFrameCount);
		~ResourceManagerVK();

		void AddStaticMesh(const std::shared_ptr<assets::StaticMeshAsset>& staticMesh);

		struct AttachmentParams
		{
			ImageSizeVK imageSize = ImageSizeVK::RenderExtentRelative;
			AttachmentType type = AttachmentType::Color;
			vk::Format format = vk::Format::eUndefined;
			uint32_t width = 0;
			uint32_t height = 0;
			float widthMult = 1.0f;
			float heightMult = 1.0f;
		};
		[[nodiscard]] ResourceID CreateOrUpdateAttachment(const AttachmentParams& params);
		[[nodiscard]] ResourceID CreateOrUpdateAttachment(const AttachmentParams& params, ResourceID id);
		[[nodiscard]] ResourceID CreateOrUpdateAttachment(const AttachmentParams& params, const std::string& name);

		struct ImageCreateParams
		{
			vk::ImageCreateInfo info;
			vk::ImageViewCreateInfo viewInfo;
			bool persistent = false;
		};
		[[nodiscard]] ResourceID CreateOrUpdateImage(const ImageCreateParams& params);
		[[nodiscard]] ResourceID CreateOrUpdateImage(const ImageCreateParams& params, ResourceID id);
		[[nodiscard]] ResourceID CreateOrUpdateImage(const ImageCreateParams& params, const std::string& name);

		[[nodiscard]] ResourceID CreateOrUpdateBuffer(const BufferDescVK& bufferDesc, const std::string& name);

		void DestroyResource(ResourceID id);
		void DestroyResource(const std::string& name);

		void Update();

		[[nodiscard]] bool IsResourceValid(const std::string& name) const;
		[[nodiscard]] bool IsResourceValid(ResourceID id) const;

		AllocatedImage& GetImage(ResourceID id);
		AllocatedImage& GetImage(const std::string& name);

		UnifiedGeometryBuffer* GeometryBuffer() const;

	private:

		void CreateResourcesInstances();
		void DestroyResourcesInstances();

		void CreateOrUpdateAttachmentInternal(const AttachmentParams& params, ResourceID id, const std::string& name);

		void CreateOrUpdateImageInternal(const ImageCreateParams& params, ResourceID id, const std::string& name);

		void DestroyResourceInternal(ResourceID id);

		void CreateImageInstanceInternal(ResourceID instanceID, const ImageCreateParams& params);
		void DestroyImageInstanceInternal(ResourceID instanceID);

		void CalculateImageExtent(ImageSizeVK imageSize, vk::Extent3D& extent, float widthMult = 1.0f, float heightMult = 1.0f) const;

		enum class ResourceType
		{
			Image = 0,
			Buffer
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
		UnifiedGeometryBuffer* mUnifiedGeometryBuffer = nullptr;

		uint8_t mBufferedFrameCount = 0;
		std::unordered_map<ResourceID, ResourceInfo> mResourceInfo;
		std::unordered_map<std::string, ResourceID> mResourceNameToID;

		std::unordered_map<ResourceID, AllocatedImage> mInstanceImages;
		std::unordered_map<ResourceID, ImageCreateParams> mInstanceImageDescs;

		//std::unordered_map<ResourceID, AllocatedBuffer> mInstanceBuffers;
		//std::unordered_map<ResourceID, BufferDescVK> mInstanceBufferDescs;

		std::vector<std::pair<ResourceID, ImageCreateParams>> mImageInstancesToCreate;
		std::vector<std::unordered_set<ResourceID>> mImageInstancesToDestroy;
	};
}
