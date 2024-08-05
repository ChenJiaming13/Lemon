#version 450

layout (location = 0) in vec2 iPosition;
layout (location = 1) in vec3 iColor;

layout (location = 0) out vec3 vFragColor;

layout (binding = 0) uniform UniformBufferObject
{
    mat4 uModel;
    mat4 uView;
    mat4 uProjection;
} ubo;

void main()
{
    gl_Position = ubo.uProjection * ubo.uView * ubo.uModel * vec4(iPosition, 0.0, 1.0);
    // gl_Position = vec4(iPosition, 0.0, 1.0);
    vFragColor = iColor;
}