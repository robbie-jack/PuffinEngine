#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

class Camera
{
public:

	Camera();
	~Camera();

	//inline glm::vec3 GetPosition() { return position; };
	//inline glm::vec3 GetRotation() { return rotation; };

	//inline void SetPosition(glm::vec3 position_) { position = position; UpdateViewMatrix(); };
	//inline void SetRotation(glm::vec3 rotation_) { rotation = rotation_; UpdateViewMatrix(); };

	inline glm::mat4 GetPerspectiveMatrix() { return matrices.perspective; };
	inline glm::mat4 GetViewMatrix() { return matrices.view; };

	inline void SetPerspectiveMatrix(glm::mat4 perspective) { matrices.perspective = perspective; };
	inline void SetViewMatrix(glm::mat4 view) { matrices.view = view; };

	inline void SetViewCentre(glm::vec3 centre_) { view.centre = centre_; UpdateViewMatrix(); };
	inline void SetViewEye(glm::vec3 eye_) { view.eye = eye_; UpdateViewMatrix(); };
	inline void SetViewUp(glm::vec3 up_) { view.up = up_; UpdateViewMatrix(); };

	void SetPerspective(float fov_, float aspect_, float zNear_, float zFar_);

private:
	float zNear, zFar, fov;

	//glm::vec3 position;
	//glm::vec3 rotation;

	CameraMatrices matrices;
	View view;

	void UpdateViewMatrix();
};