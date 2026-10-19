#version 330 core
layout(location = 0) in vec3 aPos;
//layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in mat4 instanceMatrix;


out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 rotation;
uniform mat4 rotationX;
uniform mat4 view;
uniform mat4 projection;


void main()
{
//remember matrice are read right to left


//Normal = mat3(transpose(inverse(model))) * aNormal;
//FragPos = vec3(model * vec4(aPos, 1.0));

gl_Position = projection * view * rotationX * rotation * instanceMatrix * vec4(aPos, 1.0f);
TexCoords = aTexCoords;
}