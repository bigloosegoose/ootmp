#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;



out VS_OUT{
	vec2 texCoords;
	vec3 normal;
	vec3 position;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	vs_out.texCoords = aTexCoords;
	vs_out.normal = aNormal;
	vs_out.position = aPos;


	vs_out.normal = mat3(transpose(inverse(model))) * aNormal;
	vs_out.position = vec3(model * vec4(aPos, 1.0));
	gl_Position = model* projection * view * vec4(aPos,1.0);
}