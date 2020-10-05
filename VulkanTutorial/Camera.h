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

		//struct View
		//{
		//	glm::vec3 eye;
		//	glm::vec3 centre;
		//	glm::vec3 up;
		//};

		struct ViewBufferObject
		{
			alignas(16) glm::vec3 viewPos;
		};

		class Camera
		{
		public:

			Camera();
			~Camera();

			void Init(glm::vec3 position_, glm::vec3 direction_, glm::vec3 up_, float fov, float aspect, float near, float far);
			void Update(Puffin::Input::InputManager* input_manager, float delta_time);

			inline glm::vec3 GetPosition() { return position; };
			inline float GetFov() { return fov; };

			inline void SetPosition(glm::vec3 position_) { position = position_; UpdateViewMatrix(); };
			inline void SetLookAt(glm::vec3 lookat_) { lookat = lookat_; UpdateViewMatrix(); };
			inline void SetUp(glm::vec3 up_) { up = up_; UpdateViewMatrix(); };

			inline glm::mat4 GetPerspectiveMatrix() { return matrices.perspective; };
			inline glm::mat4 GetViewMatrix() { return matrices.view; };

			inline void SetPerspectiveMatrix(glm::mat4 perspective) { matrices.perspective = perspective; };
			inline void SetViewMatrix(glm::mat4 view) { matrices.view = view; };

			/*inline void SetViewEye(glm::vec3 eye_) { view.eye = eye_; viewBufferObject.viewPos = view.eye;  UpdateViewMatrix(); };
			inline void SetViewCentre(glm::vec3 centre_) { view.centre = centre_; UpdateViewMatrix(); };
			inline void SetViewUp(glm::vec3 up_) { view.up = up_; UpdateViewMatrix(); };*/

			void SetPerspective(float fov_, float aspect_, float zNear_, float zFar_);
			void UpdateFov(float fov_);

			inline ViewBufferObject GetViewBufferObject() { return viewBufferObject; };

			inline VkBuffer& GetViewBuffer(int i) { return viewBufferVector[i]; };
			inline VmaAllocation& GetViewAllocation(int i) { return viewAllocationVector[i]; };

			inline std::vector<VkBuffer>& GetViewBufferVector() { return viewBufferVector; };
			inline std::vector<VmaAllocation>& GetViewAllocationVector() { return viewAllocationVector; };

		private:
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
			//View view;

			ViewBufferObject viewBufferObject;
			std::vector<VkBuffer> viewBufferVector;
			std::vector<VmaAllocation> viewAllocationVector;

			void UpdateViewMatrix();
		};
	}
}