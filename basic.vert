#version 330 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec4 inColor; // Color attribute

out vec4 fragColor; // Color to pass to fragment shader

void main()
{
    gl_Position = vec4(inPos, 0.0, 1.0);
    fragColor = inColor; // Pass the color to fragment shader
}