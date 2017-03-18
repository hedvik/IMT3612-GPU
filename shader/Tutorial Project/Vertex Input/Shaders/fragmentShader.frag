#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D textureSampler;

layout(location = 0) in vec4 vertexPosition_cameraspace;
layout(location = 1) in vec3 fragmentColor;
layout(location = 2) in vec2 fragmentTextureCoordinate;
layout(location = 3) in vec4 normal_cameraspace;
layout(location = 4) in vec4 lightPositions_cameraspace[1];

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(textureSampler, fragmentTextureCoordinate) * vec4(fragmentColor, 1);
}