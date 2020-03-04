#include "Camera.h"

Camera::Camera()
{
	//position = glm::vec3(0.0f, 0.0f, 0.0f);
	//rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	view.centre = glm::vec3(0.0f, 0.0f, 0.0f);
	view.eye = glm::vec3(0.0f, 0.0f, 0.0f);
	view.up = glm::vec3(0.0f, 0.0f, 0.0f);

	viewBufferObject.viewPos = view.centre;
}

Camera::~Camera()
{

}

void Camera::Init(glm::vec3 eye, glm::vec3 centre, glm::vec3 up, float fov, float aspect, float near, float far)
{
	SetViewEye(eye);
	SetViewCentre(centre);
	SetViewUp(up);

	SetPerspective(fov, aspect, near, far);
}

void Camera::SetPerspective(float fov_, float aspect_, float zNear_, float zFar_)
{
	fov = fov_;
	zNear = zNear_;
	zFar = zFar_;

	matrices.perspective = glm::perspective(glm::radians(fov), aspect_, zNear, zFar);
}

void Camera::UpdateViewMatrix()
{
	/*glm::mat4 rotM = glm::mat4(1.0f);
	glm::mat4 transM;

	rotM = glm::rotate(rotM, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(1.0f, 1.0f, 0.0f));
	rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(1.0f, 0.0f, 1.0f));

	transM = glm::translate(glm::mat4(1.0f), position);

	matrices.view = transM * rotM;*/

	matrices.view = glm::lookAt(view.eye, view.centre, view.up);
}