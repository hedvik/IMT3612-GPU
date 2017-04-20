#version 450
#extension GL_ARB_separate_shader_objects : enable
#define RENDERABLE_UBO		0
#define SCENE_UBO					1
#define NUM_LIGHTS                4

// Uniforms
layout(set = RENDERABLE_UBO, binding = 0) uniform RenderableUBO {
    mat4 MVP;
	mat4 ProjectionMatrix;
	mat4 ViewMatrix;
	mat4 ModelMatrix;
} renderableUBO;

layout(set = SCENE_UBO, binding = 0) uniform SceneUBO {
	vec4 lightPositions_worldspace[NUM_LIGHTS];
	vec4 lightColors[NUM_LIGHTS];
} sceneUBO;

layout(push_constant) uniform PushConsts  {
	mat4 view;
} pushConsts;

// Input values
layout(location = 0) in vec4 vertexPosition_modelspace;

// Output values. It seems like Vulkan requires these to be in separate locations to work properly
layout(location = 0) out vec4 outVertexPosition;
layout(location = 1) out vec4 outLightPosition;

void main() {
    gl_Position = renderableUBO.ProjectionMatrix * pushConsts.view * (mat4(1.0) * sceneUBO.lightPositions_worldspace[0]) * vertexPosition_modelspace;
	
	outVertexPosition =  renderableUBO.ViewMatrix * renderableUBO.ModelMatrix * vertexPosition_modelspace;
	outLightPosition = renderableUBO.ViewMatrix * renderableUBO.ModelMatrix * sceneUBO.lightPositions_worldspace[0];
}