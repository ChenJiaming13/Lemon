#version 450

layout(location = 0) in vec2 iPosition;
layout(location = 1) in vec3 iColor;

layout(location = 0) out vec3 vFragColor;

layout(push_constant) uniform Push
{
	mat4 uModel;
	mat4 uView;
	mat4 uProjection;
} push;

void main()
{
	gl_Position = push.uProjection * push.uView * push.uModel * vec4(iPosition, 0.0, 1.0);
	vFragColor = iColor;
}