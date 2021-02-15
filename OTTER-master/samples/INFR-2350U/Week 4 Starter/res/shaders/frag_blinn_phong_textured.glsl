#version 410

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

uniform sampler2D s_Diffuse;
uniform sampler2D s_Diffuse2;
uniform sampler2D s_Specular;

uniform vec3  u_AmbientCol;
uniform float u_AmbientStrength;

uniform vec3  u_LightPos;
uniform vec3  u_LightCol;
uniform float u_AmbientLightStrength;
uniform float u_SpecularLightStrength;
uniform float u_Shininess;

//Uniofrm Variable toggles
uniform int u_AmbientToggle;
uniform int u_SpecularToggle;
uniform int	 u_LightingOff;
uniform int u_AmbientAndSpecToggle;
//uniform int u_CustomShaderToggle;
uniform int u_TextureToggle;
uniform int u_BloomToggle;


// NEW in week 7, see https://learnopengl.com/Lighting/Light-casters for a good reference on how this all works, or
// https://developer.valvesoftware.com/wiki/Constant-Linear-Quadratic_Falloff
uniform float u_LightAttenuationConstant;
uniform float u_LightAttenuationLinear;
uniform float u_LightAttenuationQuadratic;

uniform float u_TextureMix;

uniform vec3  u_CamPos;

out vec4 frag_color;

// https://learnopengl.com/Advanced-Lighting/Advanced-Lighting
void main() {
	// Lecture 5
	vec3 ambient = u_AmbientLightStrength * u_LightCol;

	// Diffuse
	vec3 N = normalize(inNormal);
	vec3 lightDir = normalize(u_LightPos - inPos);

	float dif = max(dot(N, lightDir), 0.0);
	vec3 diffuse = dif * u_LightCol;// add diffuse intensity

	//Attenuation
	float dist = length(u_LightPos - inPos);
	float attenuation = 1.0f / (
		u_LightAttenuationConstant + 
		u_LightAttenuationLinear * dist +
		u_LightAttenuationQuadratic * dist * dist);

	// Specular
	vec3 viewDir  = normalize(u_CamPos - inPos);
	vec3 h        = normalize(lightDir + viewDir);

	// Get the specular power from the specular map
	float texSpec = 1.0f;
	float spec = pow(max(dot(N, h), 0.0), u_Shininess); // Shininess coefficient (can be a uniform)
	vec3 specular = u_SpecularLightStrength * texSpec * spec * u_LightCol; // Can also use a specular color

	// Get the albedo from the diffuse / albedo map
	vec4 textureColor1 = texture(s_Diffuse, inUV);
	vec4 textureColor2 = texture(s_Diffuse2, inUV);
	vec4 textureColor = mix(textureColor1, textureColor2, u_TextureMix);

	vec3 result = inColor * textureColor.rgb;

	//No lighting
	if(u_LightingOff == 1)
	{

		result = inColor * textureColor.rgb; // Object color
	}

	//Ambient only Lighting
	if(u_AmbientToggle == 1)
	{
		//Calculation just for ambient Lighting
		result = ((u_AmbientCol * u_AmbientStrength) + (ambient) * attenuation) * inColor * textureColor.rgb;
	}

	//Specular Lighting Only

	if(u_SpecularToggle == 1)
	{
		result = (specular) * attenuation * inColor * textureColor.rgb;
	}

	//Ambient and Sepcular Lighting
	if(u_AmbientAndSpecToggle == 1)
	{
	result	= ((u_AmbientCol * u_AmbientStrength) + // global ambient light
	(ambient + diffuse + specular) * attenuation // light factors from our single light
    	) * inColor * textureColor.rgb; 
	}

	//Textures on/off
	if(u_TextureToggle == 1)
	{
		result = inColor * texSpec;
	}


	frag_color = vec4(result, textureColor.a);
}