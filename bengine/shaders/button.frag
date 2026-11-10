#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform vec4 color;
uniform bool hasTexture;

void main() 
{
	if(hasTexture)
	{
		FragColor = texture(screenTexture, TexCoords);
		//FragColor = vec4(1.0,0.0,0.0,1.0);
	}

	else
	{
		FragColor = vec4(color);
		
	}
}