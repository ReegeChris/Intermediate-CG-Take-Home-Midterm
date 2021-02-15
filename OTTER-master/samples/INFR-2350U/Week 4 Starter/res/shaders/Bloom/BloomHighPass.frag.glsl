#version 420
layout(binding = 0) uniform sampler2D s_screenTex;


out vec4 FragColor;

uniform float u_Threshold;

//uniform float u_Strength;

layout(location = 0) in vec2 inUV;


void main()
{
	vec4 color = texture(s_screenTex, inUV);

	float luminance = (color.r + color.g + color.b) / 3.0;

	if(luminance > u_Threshold)
	{
		FragColor = color;
	}
		else
	{
		FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	}



}