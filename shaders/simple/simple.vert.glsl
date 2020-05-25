#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inTangent;
layout (location = 3) in vec2 inCoord;
layout (location = 4) in vec4 inColor;

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec2 fragCoord;

layout (binding = 0) uniform CameraUniform
{
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

void main()
{
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	fragColor = inColor;
	fragCoord = inCoord;
}