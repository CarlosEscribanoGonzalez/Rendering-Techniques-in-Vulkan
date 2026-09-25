#version 460

#extension GL_ARB_shader_draw_parameters : enable
#extension GL_KHR_vulkan_glsl : enable
#define INF 1.0 / 0.0

layout( location = 0 ) in vec2 f_uvs;

layout ( set = 0, binding = 1 ) uniform sampler2D i_color;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(texture(i_color, f_uvs).xyz, 1.0);
}