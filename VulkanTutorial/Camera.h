#pragma once

#include "InputManager.h"

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

struct CameraMatrices
{
	glm::mat4 perspective;
	glm::mat4 view;
};

struct View
{
	glm::vec3 eye;
	glm::vec3 centre;
	glm::vec3 up;
};

struct ViewBufferObject
{
	alignas(16) glm::vec3 viewPos;
};

class Camera
{
public:

	Camera();
	~Camera();

	void Init(glm::vec3 eye, glm::vec3 centre, glm::vec3 up, float fov, float aspect, float near, float far, InputManager* input_manager);
	void Update();

	//inline glm::vec3 GetPosition() { return position; };
	//inline glm::vec3 GetRotation() { return rotation; };

	//inline void SetPosition(glm::vec3 position_) { position = position; UpdateViewMatrix(); };
	//inline void SetRotation(glm::vec3 rotation_) { rotation = rotation_; UpdateViewMatrix(); };

	inline glm::mat4 GetPerspectiveMatrix() { return matrices.perspective; };
	inline glm::mat4 GetViewMatrix() { return matrices.view; };

	inline void SetPerspectiveMatrix(glm::mat4 perspective) { matrices.perspective = perspective; };
	inline void SetViewMatrix(glm::mat4 view) { matrices.view = view; };

	inline void SetViewCentre(glm::vec3 centre_) { view.centre = centre_; viewBufferObject.viewPos = view.centre; UpdateViewMatrix(); };
	inline void SetViewEye(glm::vec3 eye_) { view.eye = eye_; UpdateViewMatrix(); };
	inline void SetViewUp(glm::vec3 up_) { view.up = up_; UpdateViewMatrix(); };

	void SetPerspective(float fov_, float aspect_, float zNear_, float zFar_);

	inline ViewBufferObject GetViewBufferObject() { return viewBufferObject; };

	inline VkBuffer& GetViewBuffer(int i) { return viewBufferVector[i]; };
	inline VkDeviceMemory& GetViewMemory(int i) { return viewMemoryVector[i]; };

	inline std::vector<VkBuffer>& GetViewBufferVector() { return viewBufferVector; };
	inline std::vector<VkDeviceMemory>& GetViewMemoryVector() { return viewMemoryVector; };

private:
	float zNear, zFar, fov;

	glm::vec3 position;
	//glm::vec3 rotation;

	float speed;

	CameraMatrices matrices;
	View view;

	ViewBufferObject viewBufferObject;
	std::vector<VkBuffer> viewBufferVector;
	std::vector<VkDeviceMemory> viewMemoryVector;

	InputManager* inputManager;

	void UpdateViewMatrix();
};