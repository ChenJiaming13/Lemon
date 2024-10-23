#version 450

layout(location = 0) in vec3 vFragColor;
layout(location = 1) in vec2 vTexCoord;

layout(location = 0) out vec4 oFragColor;

layout (binding = 1) uniform sampler2D uTexSampler;

void main()
{
    vec4 Color = texture(uTexSampler, vTexCoord);
    Color = Color * vec4(vFragColor, 1.0);
    oFragColor = vec4(Color);
}