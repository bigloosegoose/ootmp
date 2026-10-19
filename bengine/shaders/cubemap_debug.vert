#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // aPos doubles as both the local vertex position and the cubemap sample
    // direction -- same trick the skybox uses, just without stripping the
    // camera translation, so this cube behaves like any other object in the
    // scene and you can walk around/away from it.
    TexCoords = aPos;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
