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
	mat4 ProjectionMatrix;
	mat4 ShadowViewMatrix;
	mat4 ModelMatrix;
	vec4 lightPositions_worldspace[NUM_LIGHTS];
	vec4 lightColors[NUM_LIGHTS];
} sceneUBO;

layout(push_constant) uniform PushConsts  {
	mat4 view;
} pushConsts;

// Input values
layout(location = 0) in vec4 vertexPosition_modelspace;

// Output values.
layout(location = 0) out vec4 vertexPosition_worldspace;
layout(location = 1) out vec4 lightPosition_worldspace;

void main() {
    gl_Position = sceneUBO.ProjectionMatrix * pushConsts.view * renderableUBO.ModelMatrix * vertexPosition_modelspace;
	
	vertexPosition_worldspace = renderableUBO.ModelMatrix * vertexPosition_modelspace;
	lightPosition_worldspace = sceneUBO.lightPositions_worldspace[0];
}