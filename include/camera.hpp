// This code is mainly adapted from the book with minor changes
// check https://learnopengl.com to learn more!
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <vector>

namespace UTILS
{
    enum Camera_Movement
    {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };

    const float YAW         = -90.0f;
    const float PITCH       =   0.0f;
    const float SPEED       =   0.002f;
    const float SENSITIVITY =   0.05f;

    class Camera
    {
    private:
        float Yaw;
        float Pitch;
        glm::vec3 Up;
        glm::vec3 Right;
        glm::vec3 WorldUp;
        float MovementSpeed;
        float MouseSenditivity;

    public:
        glm::vec3 Position;
        glm::vec3 Front;

        bool focus;
        float mv_zoom;
        std::vector<bool> keyMap;
        std::vector<double> mousePos;
        bool mousePosUpdated = false;

        // constructor with vectors
        Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
               glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
               float yaw = YAW, float pitch = PITCH);
        // constructor with scaler values
        Camera(float posX, float posY, float posZ,
               float upX, float upY, float upZ,
               float yaw = YAW, float pitch = PITCH);

        // return view matrix calculated using Eular Angles and LookAt Matrix
        glm::mat4 GetViewMatrix();
        // single update function to handle both key and mouse
        void update(float deltaT, float xoffset, float yoffset);
        // reset all values
        void reset();

    private:
        // process input from keyboard
        void ProcessKeyboard(Camera_Movement direction, float deltaTime);
        // process input from mouse
        void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
        void updateCameraVectors();
    };
}
