#version 450
#extension GL_ARB_separate_shader_objects : enable

// Uniforms
layout(binding = 0) uniform RenderableUBO {
    mat4 MVP;
	mat4 ViewMatrix;
	mat4 ModelMatrix;
} renderableUBO;

//layout(set = 1, binding = 2) uniform SceneUBO {
//	vec4 lightPositions_worldspace[1];
//} sceneUBO;

// Input values
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec2 textureCoordinate;
layout(location = 3) in vec3 vertexNormal_modelspace;

// Output values
layout(location = 0) out vec4 vertexPosition_cameraspace;
layout(location = 1) out vec3 fragmentColor;
layout(location = 2) out vec2 fragmentTextureCoordinate;
layout(location = 3) out vec4 normal_cameraspace;
layout(location = 4) out vec4 lightPositions_cameraspace[1];

void main() {
    gl_Position = renderableUBO.MVP * vec4(vertexPosition_modelspace, 1.0);
    fragmentColor = vertexColor;
    fragmentTextureCoordinate = textureCoordinate;
	
	
	// Vector that goes from the vertex to the camera, in camera space.
	// In camera space, the camera is at the origin (0,0,0).
	vertexPosition_cameraspace =  renderableUBO.ViewMatrix * renderableUBO.ModelMatrix * vec4(vertexPosition_modelspace,  0);
		
	// Normal of the the vertex, in camera space
	normal_cameraspace = renderableUBO.ViewMatrix * renderableUBO.ModelMatrix * vec4(vertexNormal_modelspace, 1);
	
	// The lights position in the camera space
	int i;
	for(i = 0; i < 1; i++) {
		//lightPositions_cameraspace[i] = renderableUBO.ViewMatrix * sceneUBO.lightPositions_worldspace[i];
	} 
}