#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D asciiMap;

uniform float width;
uniform float height;

// 0 = normal, 1 = ascii, 2 = monochrome, 3 = night reading
uniform int mode;

const vec2 charSize = vec2(8.0, 8.0);
const vec3 lumaWeights = vec3(0.2126, 0.7152, 0.0722);
const vec3 nightTint = vec3(1.0, 0.82, 0.55); // warm yellowish tint: less green, much less blue


void main()
{
	vec2 screenRes = vec2(width, height);
	vec2 uv = vec2(TexCoords.x, 1.0 - TexCoords.y);


	if (mode == 0)
	{
		FragColor = vec4(texture(screenTexture, uv).rgb, 1.0);
		return;
	}

	
	if (mode == 2)
	{
		float grey = dot(texture(screenTexture, uv).rgb, lumaWeights);
		FragColor = vec4(vec3(grey), 1.0);
		return;
	}


	if (mode == 3)
	{
		vec3 c = texture(screenTexture, uv).rgb;
		FragColor = vec4(c * nightTint, 1.0);
		return;
	}

	// dividing into cells
	vec2 numCells = screenRes / charSize;

	//normalized in grid space??
	vec2 cellUV = floor(uv * numCells) / numCells;

	//luminance(greyscale)
	vec3 sceneColor = texture(screenTexture, cellUV).rgb; 
	float brightness = dot(sceneColor, lumaWeights);

	//texCoord in grid space;
	vec2 localUV = fract(uv * numCells);

	//assign a character
	float charCount = 10.0;
	float charIndex = floor(brightness * (charCount - 1.0));

	//sample the character
	vec2 mapUV = vec2((charIndex + localUV.x) / charCount, localUV.y);
	vec4 asciiPixel = texture(asciiMap, mapUV);

	FragColor = asciiPixel * vec4(sceneColor, 1.0);
}
