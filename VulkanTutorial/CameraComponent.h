#pragma once

#include "InputManager.h"

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

namespace Puffin
{
	namespace Rendering
	{
		struct CameraMatrices
		{
			glm::mat4 perspective;
			glm::mat4 view;
		};

		struct ViewBufferObject
		{
			alignas(16) glm::vec3 viewPos;
		};

		struct CameraComponent
		{
			float zNear, zFar, aspect, fov;

			glm::vec3 position;
			glm::vec3 lookat;

			glm::vec3 direction;
			glm::vec3 up;
			glm::vec3 right;

			float yaw;
			float pitch;
			float speed;

			CameraMatrices matrices;

			ViewBufferObject viewBufferObject;
			std::vector<VkBuffer> viewBuffers;
			std::vector<VmaAllocation> viewAllocations;
		};

		static void UpdatePerspective(CameraComponent& camera, float fov_, float aspect_, float zNear_, float zFar_)
		{
			camera.fov = fov_;
			camera.aspect = aspect_;
			camera.zNear = zNear_;
			camera.zFar = zFar_;

			camera.matrices.perspective = glm::perspective(glm::radians(camera.fov), camera.aspect, camera.zNear, camera.zFar);
		}

		static void UpdateFov(CameraComponent& camera, float fov_)
		{
			camera.fov = fov_;

			camera.matrices.perspective = glm::perspective(glm::radians(camera.fov), camera.aspect, camera.zNear, camera.zFar);
		}

		static void UpdateViewMatrix(CameraComponent& camera)
		{
			camera.matrices.view = glm::lookAt(camera.position, camera.position + camera.direction, camera.up);
		}
	}
}