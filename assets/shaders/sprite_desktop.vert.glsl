#version 330 core
#extension GL_ARB_explicit_uniform_location: enable

layout(location=0) in vec3 a_position; 
layout(location=1) in vec2 a_uv; 
layout(location=2) in vec4 a_color; 

out vec2 v_uv;   
out vec4 v_color;   

layout (location = 0) uniform mat4 vp;
layout (location = 1) uniform mat4 model;

void main()       
{
    gl_Position = vp * model * vec4(a_position, 1.0);
    v_uv = a_uv;
    v_color = a_color;
}
