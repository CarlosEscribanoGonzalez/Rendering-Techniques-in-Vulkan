#version 460

#extension GL_ARB_shader_draw_parameters : enable
#extension GL_KHR_vulkan_glsl : enable

layout( location = 0 ) in vec2 f_uvs;

layout ( set = 0, binding = 1) uniform sampler2D i_color1;
layout ( set = 0, binding = 2) uniform sampler2D i_color2;

layout(location = 0) out vec4 out_color;

void main() {
    vec3 color1 = texture(i_color1, f_uvs).rgb;
    vec3 color2 = texture(i_color2, f_uvs).rgb;
    out_color = vec4(color1 + color2, 1.0);
} 