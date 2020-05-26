#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec4 fragColor;
layout (location = 1) in vec2 fragCoord;

layout (location = 0) out vec4 outColor;

layout (binding = 2) uniform sampler2D texBase;
layout (binding = 3) uniform sampler2D texRough;
layout (binding = 4) uniform sampler2D texNormal;
layout (binding = 5) uniform sampler2D texOcclusion;
layout (binding = 6) uniform sampler2D texEmissive;

layout (push_constant) uniform MeshConstants
{
	float hasBase;
    float hasRough;
    float hasNormal;
    float hasOcclusion;
    float hasEmissive;
} m_constants;

void main()
{
	// outColor = vec4(1.0, 1.0, 1.0, 1.0);
	// outColor = vec4(texture(texBase, fragCoord).rgb, 1.0);
	if(m_constants.hasBase > 0)
		outColor = texture(texBase, fragCoord);
	else
		outColor = vec4(1.0, 1.0, 1.0, 1.0);
}