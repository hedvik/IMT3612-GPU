#version 450
#extension GL_ARB_separate_shader_objects : enable
#define RENDERABLE_UBO		0
#define SCENE_UBO					1
#define NUM_LIGHTS                1

// Uniforms
layout(set = RENDERABLE_UBO, binding = 0) uniform RenderableUBO {
    mat4 MVP;
	mat4 ViewMatrix;
	mat4 ModelMatrix;
} renderableUBO;

layout(set = SCENE_UBO, binding = 0) uniform SceneUBO {
	vec4 lightPositions_worldspace[NUM_LIGHTS];
	// vec4 lightColors[NUM_LIGHTS];
} sceneUBO;

// Input values
layout(location = 0) in vec4 vertexPosition_modelspace;
layout(location = 1) in vec4 vertexColor;
layout(location = 2) in vec4 textureCoordinate;
layout(location = 3) in vec4 vertexNormal_modelspace;

// Output values
layout(location = 0) out vec4 vertexPosition_cameraspace;
layout(location = 1) out vec4 fragmentColor;
layout(location = 2) out vec4 fragmentTextureCoordinate;
layout(location = 3) out vec4 normal_cameraspace;
layout(location = 4) out vec4 lightPositions_cameraspace[NUM_LIGHTS];

void main() {
    gl_Position = renderableUBO.MVP * vertexPosition_modelspace;
    fragmentColor = vertexColor;
    fragmentTextureCoordinate = textureCoordinate;
	
	// Vector that goes from the vertex to the camera, in camera space.
	// In camera space, the camera is at the origin (0,0,0).
	vertexPosition_cameraspace =  renderableUBO.ViewMatrix * renderableUBO.ModelMatrix * vertexPosition_modelspace;
		
	// Normal of the the vertex, in camera space
	normal_cameraspace = renderableUBO.ViewMatrix * renderableUBO.ModelMatrix * vertexNormal_modelspace;
	
	// The lights position in the camera space
	int i;
	for(i = 0; i < NUM_LIGHTS; i++) {
		lightPositions_cameraspace[i] = renderableUBO.ViewMatrix * sceneUBO.lightPositions_worldspace[i];
	} 
}