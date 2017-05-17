#version 450
#extension GL_ARB_separate_shader_objects : enable
#define RENDERABLE_UBO		0
#define SCENE_UBO					1
#define BINDING_SAMPLER       1
#define BINDING_MATERIAL      2
#define NUM_LIGHTS                4
#define EPSILON                       0.15
#define SHADOW_OPACITY       0.5

layout(set = RENDERABLE_UBO, binding = BINDING_SAMPLER) uniform sampler2D textureSampler;
layout(set = SCENE_UBO, binding = BINDING_SAMPLER) uniform samplerCube shadowSampler[NUM_LIGHTS];
layout(set = RENDERABLE_UBO, binding = BINDING_MATERIAL) uniform RenderableMaterial {
	float specularExponent;
	float specularGain;
	float diffuseGain;
} renderableMaterial;

layout(location = 0) in vec4 vertexPosition_cameraspace;
layout(location = 1) in vec4 fragmentColor;
layout(location = 2) in vec4 fragmentTextureCoordinate;
layout(location = 3) in vec4 normal_cameraspace;
layout(location = 4) in vec4 vertexPosition_worldspace;
layout(location = 5) in vec4 lightPosition_worldspace;
layout(location = 6) in vec4 lightPositions_cameraspace[NUM_LIGHTS];
layout(location = 12) in vec4 lightColors[NUM_LIGHTS];

layout(location = 0) out vec4 outColor;

// The color white
const vec4 WHITE = vec4(1.0, 1.0, 1.0, 1.0);

// Default material for everything. Might want this as a part of the RenderableUBO
const float ambientComponent = 0.25f;
const float diffuseComponent = 0.5f;
const vec4 specularComponent = WHITE;

// Functions
/**
 * Calculates the diffuse component of our fragment
 * @param materialDiffuseColor The diffuse material color we are using.
 * @param normal The models normal in camera space.
 * @param lightDirection The normalized direction from the fragment towards the light.
 * @param lightColor, the color of the light
 * @returns The resulting diffuse fragment.
 */
vec4 calculateDiffuseColor(vec4 materialDiffuseColor, vec4 normal, vec4 lightDirection, vec4 lightColor);

/**
 * Calculates the specular component of our fragment
 * @param materialSpecularColor The specular material color we are using.
 * @param normal The models normal in camera space.
 * @param lightDirection The normalized direction from the fragment towards the light.
 * @param specularExponent the exponent used to scale the size of the specular component.
 * @param lightColor, the color of the light
 * @returns The resulting specular fragment color.
 */
vec4 calculateSpecularColor(vec4 materialSpecularColor, vec4 normal, vec4 lightDirection, float specularExponent, vec4 lightColor);

void main() {
	
	vec4 materialDiffuseColor;
	vec4 materialAmbientColor;
	vec4 materialSpecularColor;
	
	float attenuationRadius = 500;
	
	outColor = vec4(0, 0, 0, 1); 
	vec4 coloredTexture = texture(textureSampler, fragmentTextureCoordinate.xy) * fragmentColor;
	
	// Normal of the computed fragment, in camera space
	vec4 normal = normalize(normal_cameraspace);
	
    int i;
	for(i = 0; i < NUM_LIGHTS; i++) {
		materialDiffuseColor = coloredTexture * diffuseComponent;
		materialAmbientColor = materialDiffuseColor * ambientComponent;
		materialSpecularColor = specularComponent;
			
		// Direction of the light (from the fragment to the light)
		vec4 lightDirection = lightPositions_cameraspace[i] - vertexPosition_cameraspace;
		
		// Distance between light and fragment
		float dist = length(lightDirection);
		lightDirection = normalize(lightDirection);
		vec4 diffuseColor = calculateDiffuseColor(materialDiffuseColor, normal, lightDirection, lightColors[i]);
		vec4 specularColor = calculateSpecularColor(materialSpecularColor, normal, lightDirection, renderableMaterial.specularExponent, lightColors[i]);
		
		// Light attenuation. Based on information from http://gamedev.stackexchange.com/questions/56897/glsl-light-attenuation-color-and-intensity-formula
		float attenuation = pow(clamp(1.0 - dist*dist /(attenuationRadius*attenuationRadius), 0.0, 1.0), 2);
		outColor += materialAmbientColor + attenuation*(renderableMaterial.diffuseGain * diffuseColor + specularColor * renderableMaterial.specularGain); 
	}
	
	// Shadow
	for(int i = 0; i < NUM_LIGHTS; i++) {
		vec4 lightDirection = lightPosition_worldspace - vertexPosition_worldspace;
		float sampledDistance = texture(shadowSampler[i], lightDirection.xyz).r;
		float distance = length(lightDirection);
		
		float shadow = (distance <= sampledDistance + EPSILON) ? 1.0 : SHADOW_OPACITY;
		outColor.rgb *= shadow;
	}
}

vec4 calculateDiffuseColor(vec4 materialDiffuseColor, vec4 normal, vec4 lightDirection, vec4 lightColor) {
	// Cosine of the angle between the normal and the light direction, 
	// clamped above 0
	//  - light is at the vertical of the triangle -> 1
	//  - light is perpendicular to the triangle -> 0
	//  - light is behind the triangle -> 0
	float normalLightDotProduct = dot(normal, lightDirection);
	normalLightDotProduct = clamp(normalLightDotProduct, 0.0, 1.0);
	
	// The diffuse color depends on color of the light, 
	// the normal light direction dot product
	// and the diffuse material.
	vec4 diffuseColor = lightColor  * normalLightDotProduct * materialDiffuseColor;

	return diffuseColor;
}

vec4 calculateSpecularColor(vec4 materialSpecularColor, vec4 normal, vec4 lightDirection, float specularExponent, vec4 lightColor) {
	// Eye vector (towards the camera)
	vec4 eyeDirection = vec4(0, 0, 0, 1.0f) - vertexPosition_cameraspace;
	eyeDirection = normalize(eyeDirection);
	
	// Blinn-Phong calculation of the specular light. Based on https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_shading_model#Fragment_shader
	vec4 halfDirection = normalize(lightDirection + eyeDirection);
	float specularAngle = max(dot(halfDirection, normal), 0.0);
	
	vec4 specularColor = lightColor * pow(specularAngle, specularExponent) * materialSpecularColor;

	return specularColor;
}