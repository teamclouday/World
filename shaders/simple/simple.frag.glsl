#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec4 fragColor;
layout (location = 1) in vec2 fragCoord;

layout (location = 0) out vec4 outColor;

layout (binding = 1) uniform sampler2D texBase;
layout (binding = 2) uniform sampler2D texRough;
layout (binding = 3) uniform sampler2D texNormal;
layout (binding = 4) uniform sampler2D texOcclusion;
layout (binding = 5) uniform sampler2D texEmissive;

void main()
{
	// outColor = fragColor;
	// outColor = vec4(texture(texBase, fragCoord).rgb, 1.0);
	outColor = texture(texBase, fragCoord);
}