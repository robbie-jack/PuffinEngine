#include "Camera.h"

Camera::Camera()
{
	position = glm::vec3(0.0f, 0.0f, 0.0f);
	//rotation = glm::vec3(0.0f, 0.0f, 0.0f);

	speed = 5.0f;

	/*view.centre = glm::vec3(0.0f, 0.0f, 0.0f);
	view.eye = glm::vec3(0.0f, 0.0f, 0.0f);
	view.up = glm::vec3(0.0f, 0.0f, 0.0f);*/

	//viewBufferObject.viewPos = view.eye;
	viewBufferObject.viewPos = position;
}

Camera::~Camera()
{

}

void Camera::Init(glm::vec3 position, glm::vec3 lookat, glm::vec3 up, float fov, float aspect, float near, float far)
{
	/*SetViewEye(eye);
	SetViewCentre(centre);
	SetViewUp(up);*/

	SetPosition(position);
	SetLookAt(lookat);
	SetUp(up);

	SetPerspective(fov, aspect, near, far);
}

void Camera::Update(InputManager* inputManager, float delta_time)
{
	glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);

	if (inputManager->GetAction("CamMoveLeft").state == GLFW_PRESS)
	{
		velocity.x = -speed * delta_time;
	}
	else if (inputManager->GetAction("CamMoveRight").state == GLFW_PRESS)
	{
		velocity.x = speed * delta_time;
	}
	else if (inputManager->GetAction("CamMoveRight").state == GLFW_RELEASE 
		&& inputManager->GetAction("CamMoveLeft").state == GLFW_RELEASE)
	{
		velocity.x = 0.0f;
	}

	if (inputManager->GetAction("CamMoveForward").state == GLFW_PRESS)
	{
		velocity.z = -speed * delta_time;
	}
	else if (inputManager->GetAction("CamMoveBackward").state == GLFW_PRESS)
	{
		velocity.z = speed * delta_time;
	}
	else if (inputManager->GetAction("CamMoveForward").state == GLFW_RELEASE
		&& inputManager->GetAction("CamMoveBackward").state == GLFW_RELEASE)
	{
		velocity.z = 0.0f;
	}

	if (inputManager->GetAction("CamMoveUp").state == GLFW_PRESS)
	{
		velocity.y = speed * delta_time;
	}
	else if (inputManager->GetAction("CamMoveDown").state == GLFW_PRESS)
	{
		velocity.y = -speed * delta_time;
	}
	else if (inputManager->GetAction("CamMoveUp").state == GLFW_RELEASE
		&& inputManager->GetAction("CamMoveDown").state == GLFW_RELEASE)
	{
		velocity.y = 0.0f;
	}

	position += velocity;

	UpdateViewMatrix();
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

	matrices.view = glm::lookAt(position, lookat, up);
}