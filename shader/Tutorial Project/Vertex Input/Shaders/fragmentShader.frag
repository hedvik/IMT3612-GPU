#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D textureSampler;

layout(location = 0) in vec4 vertexPosition_cameraspace;
layout(location = 1) in vec3 fragmentColor;
layout(location = 2) in vec2 fragmentTextureCoordinate;
layout(location = 3) in vec4 normal_cameraspace;
layout(location = 4) in vec4 lightPositions_cameraspace[1];

layout(location = 0) out vec4 outColor;

// The color white
const vec4 WHITE = vec4( 1.0, 1.0, 1.0, 1.0 );

vec4 lightColor = vec4(0, 0, 0, 1);

// Default material for everything
const float ambientComponent = 0.05f;
const float diffuseComponent = 0.5f;
const vec4 specularComponent = WHITE;

// Functions
/**
 * Calculates the diffuse component of our fragment
 * @param materialDiffuseColor The diffuse material color we are using.
 * @param normal The models normal in camera space.
 * @param lightDirection The normalized direction from the fragment towards the light.
 * @returns The resulting diffuse fragment.
 */
vec4 calculateDiffuseColor(vec4 materialDiffuseColor, vec4 normal, vec4 lightDirection);

/**
 * Calculates the specular component of our fragment
 * @param materialSpecularColor The specular material color we are using.
 * @param normal The models normal in camera space.
 * @param lightDirection The normalized direction from the fragment towards the light.
 * @param specularExponent the exponent used to scale the size of the specular component.
 * @returns The resulting specular fragment color.
 */
vec4 calculateSpecularColor(vec4 materialSpecularColor, vec4 normal, vec4 lightDirection, float specularExponent);

void main() {
	
	vec4 materialDiffuseColor;
	vec4 materialAmbientColor;
	vec4 materialSpecularColor;
	float specularExponent = 5.0;
	
	// Light attenuation values
	float k0 = 0.1;
	float k1 = 0.01;
	float k2;
	float minimumLight = 500;
	float radius = 300;
	
	// Initialising variables
	outColor = vec4(0, 0, 0, 1); 
	vec4 coloredTexture = vec4(0, 0, 0, 1);
	
	// The texture function uses the texture and the interpolated coordinate
	// to find the color of the current fragment.
	vec4 textureColor = texture(textureSampler, fragmentTextureCoordinate) * vec4(fragmentColor, 1);
	
	coloredTexture = textureColor * vec4(fragmentColor, 1);
	
	// Normal of the computed fragment, in camera space
	vec4 normal = normalize(normal_cameraspace);
	
    int i;
	for(i = 0; i < 1; i++) {
		materialDiffuseColor = coloredTexture * diffuseComponent;
		materialAmbientColor = materialDiffuseColor * ambientComponent;
		materialSpecularColor = specularComponent;
	
		lightColor = WHITE;
			
		// Direction of the light (from the fragment to the light)
		vec4 lightDirection = lightPositions_cameraspace[i] - vertexPosition_cameraspace;
		// Distance between light and fragment
		float dist = length(lightDirection);
		lightDirection = normalize(lightDirection);
		vec4 diffuseColor = calculateDiffuseColor(materialDiffuseColor, normal, lightDirection);
		vec4 specularColor = calculateSpecularColor(materialSpecularColor, normal, lightDirection, specularExponent);
		
		
		//light attenuation. Based on information from http://gamedev.stackexchange.com/questions/56897/glsl-light-attenuation-color-and-intensity-formula
		k2 = 1.0 / (pow(radius, 2) * minimumLight);
		float attenuation = 1.0 / (k0 + k1 * dist + k2 * pow(dist, 2));
		outColor += materialAmbientColor + attenuation*(diffuseColor + specularColor); 
	}
}

vec4 calculateDiffuseColor(vec4 materialDiffuseColor, vec4 normal, vec4 lightDirection)
{
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

vec4 calculateSpecularColor(vec4 materialSpecularColor, vec4 normal, vec4 lightDirection, float specularExponent)
{
	// Eye vector (towards the camera)
	vec4 eyeDirection = vec4(0, 0, 0, 1.0f) - vertexPosition_cameraspace;
	eyeDirection = normalize(eyeDirection);
		
	// Direction in which the triangle reflects the light
	vec4 lightReflectionDirection = reflect(-lightDirection, normal);

	// We need to make sure that it's possible for the light to hit the plane.
	// By multiplying the color with this are removing specular light bleed from behind objects.
	float normalLightDotProduct = dot(normal, lightDirection);
	normalLightDotProduct = ceil(clamp(normalLightDotProduct, 0.0, 1.0));
	
	// Cosine of the angle between the Eye vector and the Reflect vector,
	// clamped to 0
	//  - Looking into the reflection -> 1
	//  - Looking elsewhere -> < 1
	float eyeReflectionDotProduct = dot(eyeDirection, lightReflectionDirection);
	eyeReflectionDotProduct = clamp(eyeReflectionDotProduct, 0.0, 1.0);
	
	// The specular color depends on the color of the light,
	// the eye light reflection dot product by a power to control the size of the specular,
	// and our specular material.
	vec4 specularColor = lightColor * pow(eyeReflectionDotProduct, specularExponent) * materialSpecularColor * normalLightDotProduct;

	return specularColor;
}