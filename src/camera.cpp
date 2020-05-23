#include "camera.hpp"

#include "global.hpp"
extern Application* app;

using namespace UTILS;

Camera::Camera(glm::vec3 position, glm::vec3 up,
               float yaw, float pitch)
    : MovementSpeed(SPEED), MouseSenditivity(SENSITIVITY),
      Front(glm::vec3(0.0f, 0.0f, -1.0f)), focus(false),
      mv_zoom(1.0f), keyMap(6, false), mousePos(2, 0)
{
    this->Position = position;
    this->WorldUp = up;
    this->Yaw = yaw;
    this->Pitch = pitch;
    this->updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ,
               float upX, float upY, float upZ,
               float yaw, float pitch)
    : MovementSpeed(SPEED), MouseSenditivity(SENSITIVITY),
      Front(glm::vec3(0.0f, 0.0f, -1.0f)), focus(false),
      mv_zoom(1.0f), keyMap(6, false), mousePos(2, 0)
{
    this->Position = glm::vec3(posX, posY, posZ);
    this->WorldUp  = glm::vec3(upX, upY, upZ);
    this->Yaw   = yaw;
    this->Pitch = pitch;
    this->updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix()
{
    return glm::lookAt(this->Position, this->Position + this->Front, this->Up);
}

void Camera::update(float deltaT, float xoffset, float yoffset)
{
    BASE::Backend* p_backend = app->GetBackend();

    if(!this->focus)
        return;

    this->ProcessMouseMovement(xoffset, yoffset);

    if(this->keyMap[0])
        this->ProcessKeyboard(FORWARD, deltaT);
    if(this->keyMap[1])
        this->ProcessKeyboard(LEFT, deltaT);
    if(this->keyMap[2])
        this->ProcessKeyboard(BACKWARD, deltaT);
    if(this->keyMap[3])
        this->ProcessKeyboard(RIGHT, deltaT);
    if(this->keyMap[4])
        this->mv_zoom += 0.005f;
    if(this->keyMap[5])
        this->mv_zoom -= 0.005f;
    this->mv_zoom = mv_zoom > 0.0f ? mv_zoom : 0.001f;
    this->mv_zoom = mv_zoom < 5.0f ? mv_zoom : 5.0f;
}

void Camera::ProcessKeyboard(Camera_Movement direction, GLfloat deltaTime)
{
    GLfloat velocity = this->MovementSpeed * deltaTime;
    switch(direction)
    {
        case FORWARD:
            this->Position += this->Front * velocity;
            break;
        case BACKWARD:
            this->Position -= this->Front * velocity;
            break;
        case LEFT:
            this->Position -= this->Right * velocity;
            break;
        case RIGHT:
            this->Position += this->Right * velocity;
            break;
    }
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= this->MouseSenditivity;
    yoffset *= this->MouseSenditivity;

    this->Yaw   += xoffset;
    this->Pitch -= yoffset;

    if(constrainPitch)
    {
        if(this->Pitch > 89.0f)
            this->Pitch = 89.0f;
        if(this->Pitch < -89.0f)
            this->Pitch = -89.0f;
    }

    this->updateCameraVectors();
}

void Camera::reset()
{
    this->Position = glm::vec3(0.0f, 1.0f, 5.0f);
    this->WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    this->Yaw = YAW;
    this->Pitch = PITCH;
    this->MovementSpeed = SPEED;
    this->MouseSenditivity = SENSITIVITY;
    this->updateCameraVectors();
}

void Camera::updateCameraVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
    front.y = sin(glm::radians(this->Pitch));
    front.z = sin(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
    this->Front = glm::normalize(front);
    this->Right = glm::normalize(glm::cross(this->Front, this->WorldUp));
    this->Up    = glm::normalize(glm::cross(this->Right, this->Front));
}