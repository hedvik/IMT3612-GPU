#version 450
#extension GL_ARB_separate_shader_objects : enable
#define RENDERABLE_UBO		0
#define SCENE_UBO					1
#define NUM_LIGHTS                1

layout(set = RENDERABLE_UBO, binding = 1) uniform sampler2D textureSampler;

layout(location = 0) in vec4 vertexPosition_cameraspace;
layout(location = 1) in vec4 fragmentColor;
layout(location = 2) in vec4 fragmentTextureCoordinate;
layout(location = 3) in vec4 normal_cameraspace;
layout(location = 4) in vec4 lightPositions_cameraspace[NUM_LIGHTS];

layout(location = 0) out vec4 outColor;

// The color white
const vec4 WHITE = vec4( 1.0, 1.0, 1.0, 1.0 );

// Default material for everything. Might want this as a part of the RenderableUBO
const float ambientComponent = 0.25f;
const float diffuseComponent = 0.5f;
const vec4 specularComponent = vec4(1.0, 1.0, 1.0, 1.0);

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
	float specularExponent = 128.0;
	
	// Light attenuation values. Might want to send these as tweakable uniforms for the sake of testing
	float k0 = 0.01;
	float k1;
	float minimumLight = 500;
	float radius = 300;
	
	float specularGain = 1.0;
	float diffuseGain = 1;
	
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
		vec4 diffuseColor = calculateDiffuseColor(materialDiffuseColor, normal, lightDirection, WHITE);
		vec4 specularColor = calculateSpecularColor(materialSpecularColor, normal, lightDirection, specularExponent, WHITE);
		
		// Light attenuation. Based on information from http://gamedev.stackexchange.com/questions/56897/glsl-light-attenuation-color-and-intensity-formula
		k1 = 1.0 / (pow(radius, 2) * minimumLight);
		float attenuation = 1.0 / (1.0 + k0 * dist + k1 * pow(dist, 2));
		outColor += materialAmbientColor + attenuation*(diffuseGain * diffuseColor + specularColor * specularGain); 
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
	
	// Blinn-Phong calculation of the specular light
	vec4 halfDirection = normalize(lightDirection + eyeDirection);
	float specularAngle = max(dot(halfDirection, normal), 0.0);
	
	vec4 specularColor = lightColor * pow(specularAngle, specularExponent) * materialSpecularColor;

	return specularColor;
}