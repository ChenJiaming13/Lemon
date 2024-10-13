#version 450

layout(location = 0) in vec3 vFragColor;

layout(location = 0) out vec4 oFragColor;

void main()
{
    oFragColor = vec4(vFragColor, 1.0);
}