#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D asciiMap;

uniform float width;
uniform float height;

const vec2 charSize = vec2(8.0, 8.0);
vec2 screenRes = vec2(width, height);

void main()
{
	// dividing into cells
	vec2 numCells = screenRes / charSize;

	//normalized in grid space??
	vec2 cellUV = floor(TexCoords * numCells) / numCells; 

	//luminance(greyscale)
	vec3 sceneColor = texture(screenTexture, cellUV).rgb; 
	float brightness = dot(sceneColor, vec3(0.2126, 0.7152, 0.0722));

	//texCoord in grid space;
	vec2 localUV = fract(TexCoords * numCells);

	//assign a character
	float charCount = 10.0;
	float charIndex = floor(brightness * (charCount - 1.0));

	//sample the character
	vec2 mapUV = vec2((charIndex + localUV.x) / charCount, localUV.y);
	vec4 asciiPixel = texture(asciiMap, mapUV);

	FragColor = asciiPixel * vec4(sceneColor, 1.0);
}