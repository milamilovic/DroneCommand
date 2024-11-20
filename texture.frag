#version 330 core

in vec2 chTex;
out vec4 outCol;

uniform sampler2D uTex;

void main()
{
    vec4 texColor = texture(uTex, chTex);
    vec4 tint = vec4(0.0, 0.7, 0.5, 1.0);
    outCol = mix(texColor, tint, 0.3); // 20% green over texture
}
