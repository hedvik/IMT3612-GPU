#version 450
#extension GL_ARB_separate_shader_objects : enable
#define RENDERABLE_UBO		0
#define SCENE_UBO					1
#define BINDING_SAMPLER       1
#define BINDING_MATERIAL      2
#define NUM_LIGHTS                4

layout (location = 0) out vec4 outFragColor;

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec4 inLightPos;

void main()  {
	// Store distance to light as 32 bit float value
    vec3 lightVec = inPos.xyz - inLightPos.xyz;
    outFragColor = vec4(length(lightVec), 0, 0, 1);
}