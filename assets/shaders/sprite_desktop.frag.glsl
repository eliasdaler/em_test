#version 330 core
#extension GL_ARB_explicit_uniform_location: enable

in vec2 v_uv;
in vec4 v_color;

layout (location=2) uniform sampler2D tex;

layout (location=0) out vec4 o_color;

void main()
{
    vec4 col = texture(tex, v_uv);

    // Mom: we have sRGB at home:
    o_color = pow(col, vec4(1.f / 2.2f));
} 
