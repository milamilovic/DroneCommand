#version 330 core

in vec4 fragColor; // Incoming color from the vertex shader
out vec4 outCol;

void main()
{
    outCol = fragColor; // Use the color passed from vertex shader
}