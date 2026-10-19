#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

vec2 finalTexCoords = vec2(TexCoords.x, TexCoords.y);

void main()
{

	

	FragColor = texture(screenTexture, finalTexCoords);
}